/**
    Push to channel
*/

#ifndef PUSH_H
#define PUSH_H
#include "channel_descriptor.h"
#include "communicator.h"

/**
 * @brief SMI_OpenSendChannel
 * @param count number of data elements to send
 * @param data_type type of the data element
 * @param destination rank of the destination
 * @param port port number
 * @param comm communicator
 * @return channel descriptor
 */
SMI_Channel SMI_Open_send_channel(int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm)
{
    SMI_Channel chan;
    //setup channel descriptor
    chan.port=(char)port;
    chan.message_size=(unsigned int)count;
    chan.data_type=data_type;
    chan.op_type=SMI_SEND;
    chan.receiver_rank=(char)destination;
    //At the beginning, the sender can sends as many data items as the buffer size
    //in the receiver allows
    switch(data_type)
    {
        case(SMI_CHAR):
            chan.elements_per_packet=SMI_CHAR_ELEM_PER_PCKT;
            chan.max_tokens=BUFFER_SIZE*28;
            break;
        case(SMI_SHORT):
            chan.elements_per_packet=SMI_SHORT_ELEM_PER_PCKT;
            chan.max_tokens=BUFFER_SIZE*14;
            break;
        case(SMI_INT):
            chan.elements_per_packet=SMI_INT_ELEM_PER_PCKT;
            chan.max_tokens=BUFFER_SIZE*7;
            break;
        case (SMI_FLOAT):
            chan.elements_per_packet=SMI_FLOAT_ELEM_PER_PCKT;
            chan.max_tokens=BUFFER_SIZE*7;
            break;
        case (SMI_DOUBLE):
            chan.elements_per_packet=SMI_DOUBLE_ELEM_PER_PCKT;
            chan.max_tokens=BUFFER_SIZE*3;
            break;
    }
    //setup header for the message
    SET_HEADER_DST(chan.net.header,chan.receiver_rank);
    SET_HEADER_PORT(chan.net.header,chan.port);
    SET_HEADER_OP(chan.net.header,SMI_SEND);
    //chan.tokens=chan.max_tokens;
#if defined P2P_RENDEZVOUS
    chan.tokens=MIN(chan.max_tokens,count);//needed to prevent the compiler to optimize-away channel connections
#else //eager transmission protocol
    chan.tokens=count+1;
#endif
    chan.receiver_rank=destination;
    chan.processed_elements=0;
    chan.packet_element_id=0;
    chan.sender_rank=comm[0];
    //chan.comm=comm;//comm is not used in this first implemenation
    return chan;
}

/**
 * @brief private function SMI_Push push a data elements in the transient channel. Data transferring can be delayed
 * @param chan
 * @param data
 * @param immediate: if true the data is immediately sent, without waiting for the completion of the network packet.
 *          In general, the user should use the athore Push definition
 */
void SMI_Push_flush(SMI_Channel *chan, void* data, bool immediate)
{

    const char chan_idx_data=cks_data_table[chan->port];
    char *conv=(char*)data;
    COPY_DATA_TO_NET_MESSAGE(chan,net,conv);
    chan->processed_elements++;
    chan->packet_element_id++;
    chan->tokens--;
    //send the network packet if it full or we reached the message size
    if(chan->packet_element_id==chan->elements_per_packet || immediate || chan->processed_elements==chan->message_size)
    {
        SET_HEADER_NUM_ELEMS(chan->net.header,chan->packet_element_id);
        chan->packet_element_id=0;
        write_channel_intel(cks_data_channels[chan_idx_data],chan->net);
    }
    //This fence is not mandatory, the two channel operations can be
    //performed independently
    // mem_fence(CLK_CHANNEL_MEM_FENCE);

    if(chan->tokens==0)
    {
        //receives also with tokens=0
        //wait until the message arrives
        const char chan_idx_control=ckr_control_table[chan->port];
        SMI_Network_message mess=read_channel_intel(ckr_control_channels[chan_idx_control]);
        unsigned int tokens=*(unsigned int *)mess.data;
        //printf("PUSH: Remaining %u data elements, received %u tokens\n",chan->message_size-chan->processed_elements, tokens);
        chan->tokens+=tokens; //tokens
    }
}

/**
 * @brief SMI_Push push a data elements in the transient channel. The actual ata transferring can be delayed
 * @param chan pointer to the channel descriptor of the transient channel
 * @param data pointer to the data that can be sent
 */
void SMI_Push(SMI_Channel *chan, void* data)
{
    SMI_Push_flush(chan,data,false);
}

#endif //ifndef PUSH_H

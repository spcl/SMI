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
            chan.max_tokens=BUFFER_SIZE*SMI_CHAR_ELEM_PER_PCKT;
            break;
        case(SMI_SHORT):
            chan.elements_per_packet=SMI_SHORT_ELEM_PER_PCKT;
            chan.max_tokens=BUFFER_SIZE*SMI_SHORT_ELEM_PER_PCKT;
            break;
        case(SMI_INT):
            chan.elements_per_packet=SMI_INT_ELEM_PER_PCKT;
            chan.max_tokens=BUFFER_SIZE*SMI_INT_ELEM_PER_PCKT;
            break;
        case (SMI_FLOAT):
            chan.elements_per_packet=SMI_FLOAT_ELEM_PER_PCKT;
            chan.max_tokens=BUFFER_SIZE*SMI_FLOAT_ELEM_PER_PCKT;
            break;
        case (SMI_DOUBLE):
            chan.elements_per_packet=SMI_DOUBLE_ELEM_PER_PCKT;
            chan.max_tokens=BUFFER_SIZE*SMI_DOUBLE_ELEM_PER_PCKT;
            break;
    }
    //setup header for the message
    SET_HEADER_DST(chan.net.header,chan.receiver_rank);
    SET_HEADER_PORT(chan.net.header,chan.port);
    SET_HEADER_OP(chan.net.header,SMI_SEND);
#if defined P2P_RENDEZVOUS
    chan.tokens=MIN(chan.max_tokens,count);//needed to prevent the compiler to optimize-away channel connections
#else //eager transmission protocol
    chan.tokens=count;  //in this way, the last rendezvous is done at the end of the message. This is needed to prevent the compiler to cut-away internal FIFO buffer connections
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
void SMI_Push_flush(SMI_Channel *chan, void* data, int immediate)
{
    // implemented in codegen
}

/**
 * @brief SMI_Push push a data elements in the transient channel. The actual ata transferring can be delayed
 * @param chan pointer to the channel descriptor of the transient channel
 * @param data pointer to the data that can be sent
 */
void SMI_Push(SMI_Channel *chan, void* data)
{
    // implemented in codegen
}

#endif //ifndef PUSH_H

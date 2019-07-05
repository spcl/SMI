/**
    Pop from channel
*/

#ifndef POP_H
#define POP_H
#include "channel_descriptor.h"
#include "communicator.h"


/**
 * @brief SMI_Open_receive_channel opens a receive transient channel
 * @param count
 * @param data_type
 * @param source
 * @param port
 * @return
 */
SMI_Channel SMI_Open_receive_channel(unsigned int count, SMI_Datatype data_type, unsigned int source, unsigned int port, SMI_Comm comm)
{
    SMI_Channel chan;
    //setup channel descriptor
    chan.port=(char)port;
    chan.sender_rank=(char)source;
    chan.message_size=count;
    chan.data_type=data_type;
    chan.op_type=SMI_RECEIVE;
    switch(data_type)
    {
        case(SMI_CHAR):
            chan.size_of_type=1;
            chan.elements_per_packet=28;
            chan.max_tokens=BUFFER_SIZE*28;
            break;
        case(SMI_SHORT):
            chan.size_of_type=2;
            chan.elements_per_packet=14;
            chan.max_tokens=BUFFER_SIZE*14;
            break;
        case(SMI_INT):
            chan.size_of_type=4;
            chan.elements_per_packet=7;
            chan.max_tokens=BUFFER_SIZE*7;
            break;
        case (SMI_FLOAT):
            chan.size_of_type=4;
            chan.elements_per_packet=7;
            chan.max_tokens=BUFFER_SIZE*7;
            break;
        case (SMI_DOUBLE):
            chan.size_of_type=8;
            chan.elements_per_packet=3;
            chan.max_tokens=BUFFER_SIZE*3;
            break;
         //TODO add more data types
    }
    chan.tokens=MIN(chan.max_tokens/((unsigned int)8),count); //needed to prevent the compiler to optimize-away channel connections
    //The receiver sends tokens to the sender once every chan.max_tokens/8 received data elements
    //chan.tokens=chan.max_tokens/((unsigned int)8);
    SET_HEADER_NUM_ELEMS(chan.net.header,0);    //at the beginning no data
    chan.packet_element_id=0; //data per packet
    chan.processed_elements=0;
    chan.sender_rank=chan.sender_rank;
    chan.receiver_rank=comm[0];
    //comm is not directly used in this first implementation
    return chan;
}
/**
 * @brief SMI_Pop: receive a data element. Returns only when data arrives
 * @param chan pointer to the transient channel descriptor
 * @param data pointer to the target variable that, on return, will contain the data element
 */
void SMI_Pop(SMI_Channel *chan, void *data)
{
    //in this case we have to copy the data into the target variable
    if(chan->packet_element_id==0)
    {   //no data to be unpacked...receive from the network
        const char chan_idx_data=ckr_data_table[chan->port];
        chan->net=read_channel_intel(ckr_data_channels[chan_idx_data]);
    }
    char * ptr=chan->net.data+(chan->packet_element_id)*chan->size_of_type;
    chan->packet_element_id++;                      
    if(chan->packet_element_id==GET_HEADER_NUM_ELEMS(chan->net.header))
        chan->packet_element_id=0;
    chan->processed_elements++;
    //create data element
    if(chan->data_type==SMI_CHAR)
        *(char *)data= *(char*)(ptr);
    if(chan->data_type==SMI_SHORT)
        *(short *)data= *(short*)(ptr);
    if(chan->data_type==SMI_INT)
        *(int *)data= *(int*)(ptr);
    if(chan->data_type==SMI_FLOAT)
        *(float *)data= *(float*)(ptr);
    if(chan->data_type==SMI_DOUBLE)
        *(double *)data= *(double*)(ptr);
    chan->tokens--;
    //This is used to prevent this funny compiler to re-oder the two *_channel_intel operations
   // mem_fence(CLK_CHANNEL_MEM_FENCE);

    if(chan->tokens==0)
    {
        //At this point, the sender has still max_tokens*7/8 tokens: we have to consider this while we send
        //the new tokens to it
        unsigned int sender=((int)((int)chan->message_size-(int)chan->processed_elements-(int)chan->max_tokens*7/8)) < 0 ? 0: chan->message_size-chan->processed_elements-chan->max_tokens*7/8;
        chan->tokens=(unsigned int)(MIN(chan->max_tokens/8, sender)); //b/2
        //printf("POP: Remaining %u data elements, sending %u tokens (MIN %d, %d)\n",(int)chan->message_size-(int)chan->processed_elements, chan->tokens,chan->max_tokens/8, MAX((int)chan->message_size-(int)chan->processed_elements-(int)chan->max_tokens*7/8,0));
        const char chan_idx_control=cks_control_table[chan->port];
        SMI_Network_message mess;
        *(unsigned int*)mess.data=chan->tokens;
        SET_HEADER_DST(mess.header,chan->sender_rank);
        SET_HEADER_PORT(mess.header,chan->port);
        SET_HEADER_OP(mess.header,SMI_SYNCH);
        write_channel_intel(cks_control_channels[chan_idx_control],mess);
    }
}

#endif //ifndef POP_H

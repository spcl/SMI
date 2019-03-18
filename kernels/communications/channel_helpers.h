#ifndef CHANNEL_HELPERS_H
#define CHANNEL_HELPERS_H
#include "../datatypes/channel_descriptor.h"
/**
    Channel helpers
*/

/**
 * @brief open_channel Open a communication channel
 * @param my_rank rank of the opener
 * @param pair_rank pair rank
 * @param tag tag of the communication
 * @param message_size
 * @param type data type
 * @param op_type operation type
 * @return
 */
SMI_Channel SMI_OpenChannel(char my_rank, char pair_rank, char tag, uint message_size, SMI_DataType type, SMI_OperationType op_type)
{
    SMI_Channel chan;
    //setup channel descriptor
    chan.tag=tag;
    chan.message_size=message_size;
    chan.data_type=type;
    chan.op_type=op_type;

    switch(type)
    {
        case(SMI_INT):
            chan.size_of_type=4;
            chan.elements_per_packet=6;
            break;
        case (SMI_FLOAT):
            chan.size_of_type=4;
            chan.elements_per_packet=6;
            break;
        case (SMI_DOUBLE):
            chan.size_of_type=8;
            chan.elements_per_packet=3;
            break;
         //TODO add more data types
    }
    if(op_type == SMI_SEND)
    {
        //setup header for the message
        SET_HEADER_DST(chan.net.header,pair_rank);
        SET_HEADER_SRC(chan.net.header,my_rank);
        SET_HEADER_TAG(chan.net.header,tag);
        SET_HEADER_OP(chan.net.header,SMI_SEND);

        chan.sender_rank=my_rank;
        chan.receiver_rank=pair_rank;
        chan.processed_elements=0;
        chan.packet_element_id=0;
    }
    else
        if(op_type == SMI_RECEIVE)
        {
            chan.packet_element_id=chan.elements_per_packet; //data per packet
            chan.processed_elements=0;
            chan.sender_rank=pair_rank;
            chan.receiver_rank=my_rank;

            //no need here to setup the header of the message
        }


    return chan;
}

#endif // ifndef CHANNEL_HELPERS

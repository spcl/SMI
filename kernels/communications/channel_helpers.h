#ifndef CHANNEL_HELPERS_H
#define CHANNEL_HELPERS_H
#include "../datatypes/channel_descriptor.h"
/**
    Channel helpers
*/

/**
 * @brief SMI_OpenSendChannel
 * @param count
 * @param data_type
 * @param destination
 * @param tag
 * @return
 */
SMI_Channel SMI_OpenSendChannel(uint count, SMI_DataType data_type, char destination, char tag)
{
    SMI_Channel chan;
    //setup channel descriptor
    chan.tag=tag;
    chan.message_size=count;
    chan.data_type=data_type;
    chan.op_type=SMI_SEND;

    switch(data_type)
    {
        case(SMI_INT):
            chan.size_of_type=4;
            chan.elements_per_packet=7;
            break;
        case (SMI_FLOAT):
            chan.size_of_type=4;
            chan.elements_per_packet=7;
            break;
        case (SMI_DOUBLE):
            chan.size_of_type=8;
            chan.elements_per_packet=3;
            break;
         //TODO add more data types
    }

    //setup header for the message
    SET_HEADER_DST(chan.net.header,destination);
    //SET_HEADER_SRC(chan.net.header,my_rank);
    SET_HEADER_TAG(chan.net.header,tag);
    SET_HEADER_OP(chan.net.header,SMI_SEND);

    chan.receiver_rank=destination;
    chan.processed_elements=0;
    chan.packet_element_id=0;


    return chan;
}

SMI_Channel SMI_OpenReceiveChannel(uint count, SMI_DataType data_type, char source, char tag)
{
    SMI_Channel chan;
    //setup channel descriptor
    chan.tag=tag;
    chan.message_size=count;
    chan.data_type=data_type;
    chan.op_type=SMI_RECEIVE;

    switch(data_type)
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


    SET_HEADER_NUM_ELEMS(chan.net.header,0);    //at the beginning no data
    chan.packet_element_id=0; //data per packet
    chan.processed_elements=0;
    chan.sender_rank=source;
//    /chan.receiver_rank=my_rank;

    return chan;
}

#endif // ifndef CHANNEL_HELPERS

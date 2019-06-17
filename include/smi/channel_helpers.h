#ifndef CHANNEL_HELPERS_H
#define CHANNEL_HELPERS_H
#include "./channel_descriptor.h"
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
SMI_Channel SMI_Open_send_channel(uint count, SMI_Datatype data_type, uint destination, uint tag)
{
    SMI_Channel chan;
    //setup channel descriptor
    chan.tag=(char)tag;
    chan.message_size=count;
    chan.data_type=data_type;
    chan.op_type=SMI_SEND;
    chan.rendezvous=false;
    chan.receiver_rank=(char)destination;

    switch(data_type)
    {
        case(SMI_CHAR):
            chan.size_of_type=1;
            chan.elements_per_packet=28;
            chan.tokens=BUFF_SIZE*28; //init = buffer size * elememens_per_packet
            chan.max_tokens=BUFF_SIZE*28;
            break;
        case(SMI_INT):
            chan.size_of_type=4;
            chan.elements_per_packet=7;
            chan.tokens=BUFF_SIZE*7;
            chan.max_tokens=BUFF_SIZE*7;
            break;
        case (SMI_FLOAT):
            chan.size_of_type=4;
            chan.elements_per_packet=7;
            chan.tokens=BUFF_SIZE*7;
            chan.max_tokens=BUFF_SIZE*7;
            break;
        case (SMI_DOUBLE):
            chan.size_of_type=8;
            chan.elements_per_packet=3;
            chan.tokens=BUFF_SIZE*3;
            chan.max_tokens=BUFF_SIZE*3;
            break;
         //TODO add more data types
    }
    //setup header for the message
    SET_HEADER_DST(chan.net.header,chan.receiver_rank);
    //SET_HEADER_SRC(chan.net.header,my_rank);
    SET_HEADER_TAG(chan.net.header,chan.tag);
    SET_HEADER_OP(chan.net.header,SMI_SEND);

    chan.receiver_rank=destination;
    chan.processed_elements=0;
    chan.packet_element_id=0;


    return chan;
}

SMI_Channel SMI_Open_receive_channel(uint count, SMI_Datatype data_type, uint source, uint tag)
{
    SMI_Channel chan;
    //setup channel descriptor
    chan.tag=(char)tag;
    chan.sender_rank=(char)source;
    chan.message_size=count;
    chan.data_type=data_type;
    chan.op_type=SMI_RECEIVE;
    chan.rendezvous=false;


    switch(data_type)
    {
        case(SMI_CHAR):
            chan.size_of_type=1;
            chan.elements_per_packet=28;
            chan.max_tokens=BUFF_SIZE*28;
            break;
        case(SMI_INT):
            chan.size_of_type=4;
            chan.elements_per_packet=7;
            chan.max_tokens=BUFF_SIZE*7;
            break;
        case (SMI_FLOAT):
            chan.size_of_type=4;
            chan.elements_per_packet=7;
            chan.max_tokens=BUFF_SIZE*7;
            break;
        case (SMI_DOUBLE):
            chan.size_of_type=8;
            chan.elements_per_packet=3;
            chan.max_tokens=BUFF_SIZE*3;
            break;
         //TODO add more data types
    }
    chan.tokens=chan.max_tokens/8;

    SET_HEADER_NUM_ELEMS(chan.net.header,0);    //at the beginning no data
    chan.packet_element_id=0; //data per packet
    chan.processed_elements=0;
    chan.sender_rank=chan.sender_rank;
//    /chan.receiver_rank=my_rank;

    return chan;
}

#endif // ifndef CHANNEL_HELPERS

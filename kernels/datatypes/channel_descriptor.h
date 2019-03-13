#ifndef CHANNEL_DESCRIPTOR_H
#define CHANNEL_DESCRIPTOR_H
#include "network_message.h"


//TODO move this and open channel to another header
typedef enum{
    INT = 1,
    FLOAT = 2
}data_t;

typedef struct __attribute__((packed)) __attribute__((aligned(32))){
    network_message_t net;          //buffered network message
    char sender_rank;
    char receiver_rank;
    char tag;
    uint message_size;              //given in number of data elements
    uint processed_elements;        //how many data elements we have sent/received
    uint packet_element_id;         //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    data_t data_type;               //type of message
    operation_t op_type;            //type of operation
    char size_of_type;              //size of data type
    char elements_per_packet;       //number of data elements per packet
}chdesc_t;



/**
    Channel helpers
*/

chdesc_t open_channel(char my_rank, char pair_rank, char tag, uint message_size, data_t type, operation_t op_type)
{
    chdesc_t chan;
    //setup channel descriptor
    chan.tag=tag;
    chan.message_size=message_size;
    chan.data_type=type;
    chan.op_type=op_type;

    switch(type)
    {
        case(INT):
            chan.size_of_type=4;
            chan.elements_per_packet=6;
            break;
        case (FLOAT):
            chan.size_of_type=4;
            chan.elements_per_packet=6;
            break;
    }
    if(op_type == SEND)
    {
        //setup header for the message
        SET_HEADER_DST(chan.net.header,pair_rank);
        SET_HEADER_SRC(chan.net.header,my_rank);
        SET_HEADER_TAG(chan.net.header,tag);
        SET_HEADER_OP(chan.net.header,SEND);

        chan.sender_rank=my_rank;
        chan.receiver_rank=pair_rank;
        chan.processed_elements=0;
        chan.packet_element_id=0;
    }
    else
        if(op_type == RECEIVE)
        {
            chan.packet_element_id=chan.elements_per_packet; //data per packet
            chan.processed_elements=0;
            chan.sender_rank=pair_rank;
            chan.receiver_rank=my_rank;

            //no need here to setup the header of the message
        }


    return chan;
}


#endif

#ifndef CHANNEL_DESCRIPTOR_H
#define CHANNEL_DESCRIPTOR_H
/**
  @file channel_descriptor.h
  Point-to-point transient channel descriptor.
  It maintains all the informations that are necessary for performing a point-to-point communication (Push/Pop)
*/

#include "network_message.h"
#include "operation_type.h"
#include "data_types.h"
#include "communicator.h"
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


typedef struct __attribute__((packed)) __attribute__((aligned(64))){
    SMI_Network_message net;            //buffered network message
    char sender_rank;                   //rank of the sender
    char receiver_rank;                 //rank of the receiver
    char port;                          //channel port
    unsigned int message_size;          //given in number of data elements
    unsigned int processed_elements;    //how many data elements we have sent/received so far
    unsigned int packet_element_id;     //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    SMI_Datatype data_type;             //type of message
    char op_type;                       //type of operation
    char size_of_type;                  //size of data type
    char elements_per_packet;           //number of data elements per packet
    volatile unsigned int tokens;       //current number of tokens (one tokens allow the sender to transmit one data element)
    unsigned int max_tokens;            //max tokens on the sender side
}SMI_Channel;

#endif

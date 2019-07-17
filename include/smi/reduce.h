#ifndef REDUCE_H
#define REDUCE_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

/**
  @file reduce.h
  This file contains the channel descriptor, open channel, operation types,
  and communication primitive for reduce
*/


#include "data_types.h"
#include "header_message.h"
#include "network_message.h"
#include "operation_type.h"
#include "communicator.h"

typedef enum{
    SMI_ADD = 0,
    SMI_MAX = 1,
    SMI_MIN = 2
}SMI_Op;

/**
    Channel descriptor for reduce
*/
typedef struct __attribute__((packed)) __attribute__((aligned(64))){
    SMI_Network_message net;            //buffered network message
    char port;                          //Output channel for the bcast, used by the root
    char root_rank;
    char my_rank;                       //communicator infos
    char num_rank;
    unsigned int message_size;          //given in number of data elements
    unsigned int processed_elements;    //how many data elements we have sent/received
    char packet_element_id;             //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    SMI_Datatype data_type;             //type of message
    char size_of_type;                  //size of data type
    char elements_per_packet;           //number of data elements per packet
    SMI_Network_message net_2;          //buffered network message (we need two of them to remove aliasing)
    char packet_element_id_rcv;         //used by the receivers
    char reduce_op;                     //applied reduce operation
}SMI_RChannel;


/**
 * @brief SMI_Open_reduce_channel
 * @param count number of data elements to reduce
 * @param data_type type of the channel
 * @param op rapplied reduce operation
 * @param port port number
 * @param root rank of the root
 * @param comm communicator
 * @return the channel descriptor
 */
SMI_RChannel SMI_Open_reduce_channel(int count, SMI_Datatype data_type, SMI_Op op, int port, int root, SMI_Comm comm)
{
    // implemented in codegen
    SMI_RChannel chan;
    return chan;
}
SMI_RChannel SMI_Open_reduce_channel_ad(int count, SMI_Datatype data_type, SMI_Op op, int port, int root, SMI_Comm comm, int buffer_size)
{
    // fake function
    SMI_RChannel chan;
    return chan;
}

/**
 * @brief SMI_Reduce
 * @param chan pointer to the reduce channel descriptor
 * @param data_snd pointer to the data element that must be reduced
 * @param data_rcv pointer to the receiving data element  (root only)
 */
void SMI_Reduce(SMI_RChannel *chan,  void* data_snd, void* data_rcv)
{
    // implemented in codegen
}

#endif // REDUCE_H

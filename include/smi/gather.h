#ifndef GATHER_H
#define GATHER_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

/**
  @file gather.h
  This file contains the channel descriptor, open channel
  and communication primitive for gather
*/


#include "data_types.h"
#include "header_message.h"
#include "operation_type.h"
#include "network_message.h"
#include "communicator.h"

typedef struct __attribute__((packed)) __attribute__((aligned(64))){
    SMI_Network_message net;        //buffered network message
    int recv_count;                 //number of data elements that will be received by the root
    char port;
    int processed_elements_root;    //number of elements processed by the root
    char packet_element_id_rcv;     //used by the receivers
    char next_contrib;              //the rank of the next contributor
    char my_rank;
    char num_rank;
    char root_rank;
    SMI_Network_message net_2;      //buffered network message, used by root rank to send synchronization messages
    int send_count;                 //number of elements sent by each non-root ranks
    int processed_elements;         //how many data elements we have sent (non-root)
    char packet_element_id;         //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    char data_type;                 //type of message
    char size_of_type;              //size of data type
    char elements_per_packet;       //number of data elements per packet
}SMI_GatherChannel;

/**
 * @brief SMI_Open_gather_channel
 * @param send_count number of data elements transmitted by each rank
 * @param recv_count number of data elements received by root rank (i.e. num_ranks*send_count)
 * @param data_type type of the channel
 * @param port port number
 * @param root rank of the root
 * @param comm communicator
 * @return the channel descriptor
 */
SMI_GatherChannel SMI_Open_gather_channel(int send_count, int recv_count, SMI_Datatype data_type, int port, int root, SMI_Comm comm)
{
    // implemented in codegen
    SMI_GatherChannel chan;
    return chan;
}
SMI_GatherChannel SMI_Open_gather_channel_ad(int send_count, int recv_count, SMI_Datatype data_type, int port, int root, SMI_Comm comm, int buffer_size)
{
    // fake function
    SMI_GatherChannel chan;
    return chan;
}

/**
 * @brief SMI_Gather
 * @param chan pointer to the gather channel descriptor
 * @param data_snd pointer to the data element that must be sent
 * @param data_rcv pointer to the receiving data element (significant on the root rank only)
 */
void SMI_Gather(SMI_GatherChannel *chan, void* send_data, void* rcv_data)
{
    // implemented in codegen
}

#endif // GATHER_H

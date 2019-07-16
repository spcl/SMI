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
SMI_GatherChannel SMI_Open_gather_channel(int send_count,  int recv_count, SMI_Datatype data_type, int port, int root, SMI_Comm comm)
{
    SMI_GatherChannel chan;
    chan.port=(char)port;
    chan.send_count=send_count;
    chan.recv_count=recv_count;
    chan.data_type=data_type;
    chan.my_rank=(char)SMI_Comm_rank(comm);
    chan.root_rank=(char)root;
    chan.num_rank=(char)SMI_Comm_size(comm);
    chan.next_contrib=0;
     switch(data_type)
    {
        case (SMI_CHAR):
            chan.size_of_type=SMI_CHAR_TYPE_SIZE;
            chan.elements_per_packet=SMI_CHAR_ELEM_PER_PCKT;
            break;
        case(SMI_SHORT):
            chan.size_of_type=SMI_SHORT_TYPE_SIZE;
            chan.elements_per_packet=SMI_SHORT_ELEM_PER_PCKT;
            break;
        case(SMI_INT):
            chan.size_of_type=SMI_INT_TYPE_SIZE;
            chan.elements_per_packet=SMI_INT_ELEM_PER_PCKT;
            break;
        case (SMI_FLOAT):
            chan.size_of_type=SMI_FLOAT_TYPE_SIZE;
            chan.elements_per_packet=SMI_FLOAT_ELEM_PER_PCKT;
            break;
        case (SMI_DOUBLE):
            chan.size_of_type=SMI_DOUBLE_TYPE_SIZE;
            chan.elements_per_packet=SMI_DOUBLE_ELEM_PER_PCKT;
            break;
    }

    //setup header for the message
    SET_HEADER_SRC(chan.net.header,chan.my_rank);
    SET_HEADER_PORT(chan.net.header,chan.port);
    SET_HEADER_NUM_ELEMS(chan.net.header,0);
    SET_HEADER_OP(chan.net.header,SMI_SYNCH);
    //net_2 is used by the non-root ranks
    SET_HEADER_OP(chan.net_2.header,SMI_SYNCH);
    SET_HEADER_PORT(chan.net_2.header,chan.port);
    SET_HEADER_DST(chan.net_2.header,chan.root_rank);
    chan.processed_elements=0;
    chan.processed_elements_root=0;
    chan.packet_element_id=0;
    chan.packet_element_id_rcv=0;
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

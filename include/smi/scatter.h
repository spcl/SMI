#ifndef SCATTER_H
#define SCATTER_H

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

/**
    @file scatter.h
    This file contains the definition of channel descriptor,
    open channel and communication primitive for Scatter.
*/

#include "data_types.h"
#include "header_message.h"
#include "operation_type.h"
#include "network_message.h"
#include "communicator.h"



typedef struct __attribute__((packed)) __attribute__((aligned(64))){
    SMI_Network_message net;            //buffered network message
    char port;                          //port
    char root_rank;
    char my_rank;                       //rank of the caller
    char num_ranks;                     //total number of ranks
    unsigned int send_count;            //given in number of data elements
    unsigned int recv_count;            //given in number of data elements
    unsigned int processed_elements;    //how many data elements we have sent/received
    char packet_element_id;             //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    char data_type;                     //type of message
    SMI_Network_message net_2;          //buffered network message (used by non root ranks)
    char size_of_type;                  //size of data type
    char elements_per_packet;           //number of data elements per packet
    char packet_element_id_rcv;         //used by the receivers
    char next_rcv;                      //the  rank of the next receiver
    bool init;                          //true when the channel is opened, false when synchronization message has been sent
}SMI_ScatterChannel;

/**
 * @brief SMI_Open_scatter_channel opens a transient scatter channel
 * @param send_count number of data elements transmitted by root to each rank
 * @param recv_count number of data elements received by each rank 
 * @param data_type type of the channel
 * @param port port number
 * @param root rank of the root
 * @param comm communicator
 * @return the channel descriptor
 */
SMI_ScatterChannel SMI_Open_scatter_channel(int send_count, int recv_count,
        SMI_Datatype data_type, int port, int root, SMI_Comm comm);

/**
 * @brief SMI_Open_scatter_channel opens a transient scatter channel
 * @param send_count number of data elements transmitted by root to each rank
 * @param recv_count number of data elements received by each rank
 * @param data_type type of the channel
 * @param port port number
 * @param root rank of the root
 * @param comm communicator
 * @param asynch_degree the asynchronicity degree expressed in number of data elements
 * @return the channel descriptor
 */
SMI_ScatterChannel SMI_Open_scatter_channel_ad(int send_count, int recv_count,
        SMI_Datatype data_type, int port, int root, SMI_Comm comm, int asynch_degree);

/**
 * @brief SMI_Scatter
 * @param chan pointer to the scatter channel descriptor
 * @param data_snd pointer to the data element that must be sent (root only)
 * @param data_rcv pointer to the receiving data element
 */
void SMI_Scatter(SMI_ScatterChannel *chan, void* data_snd, void* data_rcv);

#endif // SCATTER_H

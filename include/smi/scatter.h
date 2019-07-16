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
 * @brief SMI_Open_scatter_channel
 * @param send_count number of data elements transmitted by root to each rank
 * @param recv_count number of data elements received by each rank 
 * @param data_type type of the channel
 * @param port port number
 * @param root rank of the root
 * @param comm communicator
 * @return the channel descriptor
 */
SMI_ScatterChannel SMI_Open_scatter_channel(int send_count,  int recv_count,
 SMI_Datatype data_type, int port, int root, SMI_Comm comm)
{
    SMI_ScatterChannel chan;
    //setup channel descriptor
    chan.send_count=(unsigned int)send_count;
    chan.recv_count=(unsigned int)recv_count;
    chan.data_type=data_type;
    chan.port=(char)port;
    chan.my_rank=(char)comm[0];
    chan.num_ranks=(char)comm[1];
    chan.root_rank=(char)root;
    chan.next_rcv=0;
    chan.init=true;
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
    if(chan.my_rank!=chan.root_rank)
    {
        //this is set up to send a "ready to receive" to the root
        SET_HEADER_OP(chan.net.header,SMI_SYNCH);
        SET_HEADER_DST(chan.net.header,chan.root_rank);
        SET_HEADER_PORT(chan.net.header,chan.port);
        SET_HEADER_SRC(chan.net.header,chan.my_rank);
    }
    else
    {
        SET_HEADER_SRC(chan.net.header,chan.root_rank);
        SET_HEADER_PORT(chan.net.header,chan.port);         //used by destination
        SET_HEADER_NUM_ELEMS(chan.net.header,0);            //at the beginning no data
        SET_HEADER_OP(chan.net.header,SMI_SYNCH);
    }

    chan.processed_elements=0;
    chan.packet_element_id=0;
    chan.packet_element_id_rcv=0;
    return chan;
}

/**
 * @brief SMI_Scatter
 * @param chan pointer to the scatter channel descriptor
 * @param data_snd pointer to the data element that must be sent (root only)
 * @param data_rcv pointer to the receiving data element
 */
void SMI_Scatter(SMI_ScatterChannel *chan, void* data_snd, void* data_rcv)
{
    // implemented in codegen
}

#endif // SCATTER_H

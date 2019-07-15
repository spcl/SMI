#ifndef BCAST_H
#define BCAST_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

/**
   @file bcast.h
   This file contains the definition of channel descriptor,
   open channel and communication primitive for Broadcast.
*/

#include "data_types.h"
#include "header_message.h"
#include "operation_type.h"
#include "network_message.h"
#include "communicator.h"

typedef struct __attribute__((packed)) __attribute__((aligned(64))){
    SMI_Network_message net;            //buffered network message
    char root_rank;
    char my_rank;                       //These two are essentially the Communicator
    char num_rank;
    char port;                          //Port number
    unsigned int message_size;          //given in number of data elements
    unsigned int processed_elements;    //how many data elements we have sent/received
    char packet_element_id;             //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    SMI_Datatype data_type;             //type of message
    SMI_Network_message net_2;          //buffered network message: used for the receiving side
    char size_of_type;                  //size of data type
    char elements_per_packet;           //number of data elements per packet
    char packet_element_id_rcv;         //used by the receivers
}SMI_BChannel;

/**
 * @brief SMI_Open_bcast_channel
 * @param count number of data elements to broadcast
 * @param data_type type of the channel
 * @param port port number
 * @param root rank of the root
 * @param comm communicator
 * @return the channel descriptor
 */
SMI_BChannel SMI_Open_bcast_channel(int count, SMI_Datatype data_type, int port, int root,  SMI_Comm comm)
{
    SMI_BChannel chan;
    //setup channel descriptor
    chan.message_size=count;
    chan.data_type=data_type;
    chan.port=(char)port;
    chan.my_rank=(char)SMI_Comm_rank(comm);
    chan.root_rank=(char)root;
    chan.num_rank=(char)SMI_Comm_size(comm);
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

    if(chan.my_rank!=chan.root_rank)
    {
        //At the beginning, send a "ready to receive" to the root
        //This is needed to not inter-mix subsequent collectives
        SET_HEADER_OP(chan.net.header,SMI_SYNCH);
        SET_HEADER_DST(chan.net.header,chan.root_rank);
        SET_HEADER_SRC(chan.net.header,chan.my_rank);
        SET_HEADER_PORT(chan.net.header,chan.port);
        const char chan_idx_control=cks_control_table[chan.port];
        write_channel_intel(cks_control_channels[chan_idx_control],chan.net);
    }
    else
    {
        SET_HEADER_OP(chan.net.header,SMI_SYNCH);           //used to signal to the support kernel that a new broadcast has begun
        SET_HEADER_SRC(chan.net.header,chan.root_rank);
        SET_HEADER_PORT(chan.net.header,chan.port);         //used by destination
        SET_HEADER_NUM_ELEMS(chan.net.header,0);            //at the beginning no data
    }
    chan.processed_elements=0;
    chan.packet_element_id=0;
    chan.packet_element_id_rcv=0;
    return chan;
}


/**
 * @brief SMI_Bcast
 * @param chan pointer to the broadcast channel descriptor
 * @param data pointer to the data element: on the root rank is the element that will be transmitted,
    on the non-root rank will be the received element
 */
void SMI_Bcast(SMI_BChannel *chan, void* data)
{
    char *conv=(char*)data;
    if(chan->my_rank==chan->root_rank)//I'm the root
    {
        const unsigned int message_size=chan->message_size;
        chan->processed_elements++;
        COPY_DATA_TO_NET_MESSAGE(chan,chan->net,conv);

        chan->packet_element_id++;
        //send the network packet if it is full or we reached the message size
        if(chan->packet_element_id==chan->elements_per_packet || chan->processed_elements==message_size)
        {
            SET_HEADER_NUM_ELEMS(chan->net.header,chan->packet_element_id);
            SET_HEADER_PORT(chan->net.header,chan->port);
            chan->packet_element_id=0;
            //offload to support kernel
            const char chan_bcast_idx=broadcast_table[chan->port];
            write_channel_intel(broadcast_channels[chan_bcast_idx],chan->net);
            SET_HEADER_OP(chan->net.header,SMI_BROADCAST);  //for the subsequent network packets
        }

    }
    else //I have to receive
    {
        if(chan->packet_element_id_rcv==0 )
        {
            const char chan_idx_data=ckr_data_table[chan->port]; //TODO: problematic for const prop. Seems to be completely wrong, no matter the port id claimed in the open channel
            chan->net_2=read_channel_intel(ckr_data_channels[chan_idx_data]);
        }

        COPY_DATA_TO_NET_MESSAGE(chan,chan->net_2,conv);

        chan->packet_element_id_rcv++;
        if( chan->packet_element_id_rcv==chan->elements_per_packet)
            chan->packet_element_id_rcv=0;
    }
}
#endif // BCAST_H

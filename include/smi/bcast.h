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
SMI_BChannel SMI_Open_bcast_channel(unsigned int count, SMI_Datatype data_type, unsigned int port, unsigned int root,  SMI_Comm comm)
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
        case(SMI_SHORT):
            chan.size_of_type=2;
            chan.elements_per_packet=14;
            break;
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
        case (SMI_CHAR):
            chan.size_of_type=1;
            chan.elements_per_packet=28;
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

void SMI_Bcast(SMI_BChannel *chan, volatile void* data)
{
    if(chan->my_rank==chan->root_rank)//I'm the root
    {
        char *conv=(char*)data;
        char *data_snd=chan->net.data;
        const unsigned int message_size=chan->message_size;
        chan->processed_elements++;

        switch(chan->data_type) //copy the data
        {
            case SMI_CHAR:
                data_snd[chan->packet_element_id]=*conv;
                break;
            case SMI_SHORT:
                #pragma unroll
                for(int jj=0;jj<2;jj++)
                    data_snd[chan->packet_element_id*2+jj]=conv[jj];
                break;
            case SMI_INT:
            case SMI_FLOAT:
                #pragma unroll
                for(int jj=0;jj<4;jj++)
                    data_snd[chan->packet_element_id*4+jj]=conv[jj];
                break;
            //TODO: add double support
           /* case SMI_DOUBLE:
                #pragma unroll
                for(int jj=0;jj<8;jj++) //copy the data
                    data_snd[chan->packet_element_id*8+jj]=conv[jj];
            break;*/

        }
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
        char *data_rcv=chan->net_2.data;
        switch(chan->data_type)
        {
            case SMI_CHAR:
            {
                char * ptr=data_rcv;
                *(char *)data= *(char*)(ptr);
                break;
            }
            case SMI_INT:
            {
                char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
                *(int *)data= *(int*)(ptr);
                break;
            }
            case SMI_FLOAT:
            {
                char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
                *(float *)data= *(float*)(ptr);
                break;
            }
            case SMI_SHORT:
            {
                char * ptr=data_rcv+(chan->packet_element_id_rcv)*2;
                *(short *)data= *(short*)(ptr);
                break;
            }
            //TODO: add double support
            /*case SMI_DOUBLE:
            {
                char * ptr=data_rcv+(chan->packet_element_id_rcv)*8;
                *(double *)data= *(double*)(ptr);
                break;
            }*/
        }
        chan->packet_element_id_rcv++;
        if( chan->packet_element_id_rcv==chan->elements_per_packet)
            chan->packet_element_id_rcv=0;
    }
}
#if 0
void SMI_Bcast_int(SMI_BChannel *chan, volatile void* data)
{
    if(chan->my_rank==chan->root_rank)//I'm the root
    {

        char *conv=(char*)data;

        char *data_snd=chan->net.data;
        const unsigned int message_size=chan->message_size;
        chan->processed_elements++;

        switch(chan->data_type) //copy the data
        {
            case SMI_CHAR:
                data_snd[chan->packet_element_id]=*conv;
                break;
            case SMI_SHORT:
                #pragma unroll
                for(int jj=0;jj<2;jj++)
                    data_snd[chan->packet_element_id*2+jj]=conv[jj];
                break;
            case SMI_INT:
            case SMI_FLOAT:
                #pragma unroll
                for(int jj=0;jj<4;jj++)
                    data_snd[chan->packet_element_id*4+jj]=conv[jj];
                break;
            //TODO: add double support
           /* case SMI_DOUBLE:
                #pragma unroll
                for(int jj=0;jj<8;jj++) //copy the data
                    data_snd[chan->packet_element_id*8+jj]=conv[jj];
            break;*/

        }
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
            const char chan_idx_data=ckr_data_table[chan->port];
            chan->net_2=read_channel_intel(ckr_data_channels[chan_idx_data]);
        }
        char *data_rcv=chan->net_2.data;
        switch(chan->data_type)
        {
            case SMI_CHAR:
            {
                char * ptr=data_rcv;
                *(char *)data= *(char*)(ptr);
                break;
            }
            case SMI_INT:
            {
                char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
                *(int *)data= *(int*)(ptr);
                break;
            }
            case SMI_FLOAT:
            {
                char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
                *(float *)data= *(float*)(ptr);
                break;
            }
            case SMI_SHORT:
            {
                char * ptr=data_rcv+(chan->packet_element_id_rcv)*2;
                *(short *)data= *(short*)(ptr);
                break;
            }
            //TODO: add double support
            /*case SMI_DOUBLE:
            {
                char * ptr=data_rcv+(chan->packet_element_id_rcv)*8;
                *(double *)data= *(double*)(ptr);
                break;
            }*/
        }
        chan->packet_element_id_rcv++;
        if( chan->packet_element_id_rcv==chan->elements_per_packet)
            chan->packet_element_id_rcv=0;
    }
}

#endif
#endif // BCAST_H

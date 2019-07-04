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
}SMI_ScatterChannel;


SMI_ScatterChannel SMI_Open_scatter_channel(unsigned int send_count,  unsigned int recv_count,
                                            SMI_Datatype data_type, unsigned int port, unsigned int root, SMI_Comm comm)
{
    SMI_ScatterChannel chan;
    //setup channel descriptor
    chan.send_count=send_count;
    chan.recv_count=recv_count;
    chan.data_type=data_type;
    chan.port=(char)port;
    chan.my_rank=(char)comm[0];
    chan.num_ranks=(char)comm[1];
    chan.root_rank=(char)root;
    chan.next_rcv=0;
    switch(data_type)
    {
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

    //setup header for the message
    if(chan.my_rank!=chan.root_rank)
    {
        //this is set up to send a "ready to receive" to the root
        SET_HEADER_OP(chan.net.header,SMI_SYNCH);
        SET_HEADER_DST(chan.net.header,chan.root_rank);
        SET_HEADER_PORT(chan.net.header,chan.port);
        const char chan_idx_control=cks_control_table[chan.port];
        write_channel_intel(cks_control_channels[chan_idx_control],chan.net); 
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


void SMI_Scatter(SMI_ScatterChannel *chan, void* send_data, void* rcv_data)
{
    //take here the pointers to send/recv data to avoid fake dependencies
    const char elem_per_packet=chan->elements_per_packet;
    if(chan->my_rank==chan->root_rank)//I'm the root
    {
        //the root is responsible for splitting the data in packets
        //and set the right receviver.
        //If the receiver is itself it has to set the rcv_data accordingly
        char *conv=(char*)send_data;
        char *data_snd=chan->net.data;
        const unsigned int message_size=chan->send_count;
        chan->processed_elements++;
        switch(chan->data_type)
        {
            case SMI_CHAR:
                if(chan->next_rcv==chan->my_rank)
                    ((char *)(rcv_data))[0]=*conv;
                else
                    data_snd[chan->packet_element_id]=*conv;
            break;
            case SMI_INT:
            case SMI_FLOAT:
                #pragma unroll
                for(int jj=0;jj<4;jj++) //copy the data
                    if(chan->next_rcv==chan->my_rank)
                        ((char *)(rcv_data))[jj]=conv[jj]; //in this case is the root
                    else
                        data_snd[chan->packet_element_id*4+jj]=conv[jj];
            break;
            case SMI_DOUBLE:
                #pragma unroll
                for(int jj=0;jj<8;jj++) //copy the data
                    if(chan->next_rcv==chan->my_rank)
                         ((char *)(rcv_data))[jj]=conv[jj];
                    else
                        data_snd[chan->packet_element_id*8+jj]=conv[jj];
            break;
        }

        chan->packet_element_id++;
        //split this in packets holding send_count elements
        if(chan->packet_element_id==elem_per_packet || chan->processed_elements==message_size) //send it if packet is filled or we reached the message size
        {

            SET_HEADER_NUM_ELEMS(chan->net.header,chan->packet_element_id);
            SET_HEADER_PORT(chan->net.header,chan->port); 
            SET_HEADER_DST(chan->net.header,chan->next_rcv);
            //offload to scatter kernel

            if(chan->next_rcv!=chan->my_rank)
            {
                const char chan_scatter_idx=scatter_table[chan->port];
                write_channel_intel(scatter_channels[chan_scatter_idx],chan->net);
                SET_HEADER_OP(chan->net.header,SMI_SCATTER);
            }

            chan->packet_element_id=0;
            if(chan->processed_elements==message_size)
            {   //we finished the data that need to be sent to this receiver
                chan->processed_elements=0;
                chan->next_rcv++;
            }
        }
    }
    else //non-root rank: receive and unpack
    {
        if(chan->packet_element_id_rcv==0)
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
                *(char *)rcv_data= *(char*)(ptr);
                break;
            }
            case SMI_INT:
            {
                 char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
                 *(int *)rcv_data= *(int*)(ptr);
                break;
            }
            case SMI_FLOAT:
            {
                 char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
                 *(float *)rcv_data= *(float*)(ptr);
                break;
            }
            case SMI_DOUBLE:
            {
                char * ptr=data_rcv+(chan->packet_element_id_rcv)*8;
                *(double *)rcv_data= *(double*)(ptr);
                break;
            }
        }
        chan->packet_element_id_rcv++;                 
        if( chan->packet_element_id_rcv==elem_per_packet)
             chan->packet_element_id_rcv=0;
    }

}

#endif // SCATTER_H

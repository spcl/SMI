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
        case (SMI_SHORT):
            chan.size_of_type=2;
            chan.elements_per_packet=14;
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
    if(chan->my_rank==chan->root_rank)//I'm the root
    {
        //we can't offload this part to the kernel, by passing a network message
        //because it will replies to the root once every elem_per_packet cycles
        //the root is responsible for receiving data from a contributor, after sending it a request
        //for sending the data. If the contributor is itself it has to set the rcv_data accordingly
        const int message_size=chan->recv_count;

        if(chan->next_contrib!=chan->my_rank && chan->processed_elements_root==0 )//at the beginning we have to send the request
        {
            // SMI_Network_message request;
            SET_HEADER_OP(chan->net_2.header,SMI_SYNCH);
            SET_HEADER_NUM_ELEMS(chan->net_2.header,1); //this is used in the support kernel
            SET_HEADER_DST(chan->net_2.header,chan->next_contrib);
            SET_HEADER_PORT(chan->net_2.header,chan->port);
            const char chan_idx_control=cks_control_table[chan->port];
            write_channel_intel(cks_control_channels[chan_idx_control],chan->net_2);
        }
        //This fence is not necessary, the two channel operation are independent
        //and we don't need any ordering between them
        //mem_fence(CLK_CHANNEL_MEM_FENCE);

        //receive the data
        if(chan->packet_element_id_rcv==0 && chan->next_contrib!=chan->my_rank) {
            const char chan_idx_data=ckr_data_table[chan->port];
            chan->net=read_channel_intel(ckr_data_channels[chan_idx_data]);
        }

        char *data_recvd=chan->net.data;
        char *conv=(char*)send_data;
        switch(chan->data_type)
        {
            case SMI_CHAR:
               #pragma unroll
                for (int ee = 0; ee < SMI_CHAR_ELEM_PER_PCKT; ee++) {
                    if (ee == chan->packet_element_id_rcv) {
                       #pragma unroll
                        for (int jj = 0; jj < SMI_CHAR_TYPE_SIZE; jj++) {
                            if(chan->next_contrib!=chan->root_rank)     //not my turn
                                ((char *)rcv_data)[jj] = data_recvd[(ee * SMI_CHAR_TYPE_SIZE) + jj];
                            else
                                ((char *)rcv_data)[jj] = conv[jj];
                        }
                    }
                }
                break;
            case SMI_SHORT:
               #pragma unroll
                for (int ee = 0; ee < SMI_SHORT_ELEM_PER_PCKT; ee++) {
                    if (ee == chan->packet_element_id_rcv) {
                       #pragma unroll
                        for (int jj = 0; jj < SMI_SHORT_TYPE_SIZE; jj++) {
                            if(chan->next_contrib!=chan->root_rank)     //not my turn
                                ((char *)rcv_data)[jj] = data_recvd[(ee * SMI_SHORT_TYPE_SIZE) + jj];
                            else
                                ((char *)rcv_data)[jj] = conv[jj];
                        }
                    }
                }
            case SMI_INT:
            case SMI_FLOAT:
               #pragma unroll
                for (int ee = 0; ee < SMI_INT_ELEM_PER_PCKT; ee++) {
                    if (ee == chan->packet_element_id_rcv) {
                       #pragma unroll
                        for (int jj = 0; jj < SMI_INT_TYPE_SIZE; jj++) {
                            if(chan->next_contrib!=chan->root_rank)     //not my turn
                                ((char *)rcv_data)[jj] = data_recvd[(ee * SMI_INT_TYPE_SIZE) + jj];
                            else
                                ((char *)rcv_data)[jj] = conv[jj];
                        }
                    }
                }
                break;
            case SMI_DOUBLE:
               #pragma unroll
                for (int ee = 0; ee < SMI_DOUBLE_ELEM_PER_PCKT; ee++) {
                    if (ee == chan->packet_element_id_rcv) {
                       #pragma unroll
                        for (int jj = 0; jj < SMI_DOUBLE_TYPE_SIZE; jj++) {
                            if(chan->next_contrib!=chan->root_rank)     //not my turn
                                ((char *)rcv_data)[jj] = data_recvd[(ee * SMI_DOUBLE_TYPE_SIZE) + jj];
                            else
                                ((char *)rcv_data)[jj] = conv[jj];
                        }
                    }
                }
                break;
        }
/*
        switch(chan->data_type)
        {
            case SMI_CHAR:
                {
                    if(chan->next_contrib!=chan->root_rank)
                    {
                        char * ptr=chan->net.data;
                        *(char *)rcv_data= *(char*)(ptr);
                    }
                    else
                        *(char *)rcv_data= *(char*)(send_data);
                    break;
                }
            case SMI_INT:
                {
                    if(chan->next_contrib!=chan->root_rank)
                    {
                        char * ptr=chan->net.data+(chan->packet_element_id_rcv)*4;
                        *(int *)rcv_data= *(int*)(ptr);
                    }
                    else
                    {
                        //  printf("copying data to root\n");
                        *(int *)rcv_data= *(int*)(send_data);
                    }
                    break;
                }
            case SMI_FLOAT:
                {
                    if(chan->next_contrib!=chan->root_rank)
                    {
                        char * ptr=chan->net.data+(chan->packet_element_id_rcv)*4;
                        *(float *)rcv_data= *(float*)(ptr);
                    }
                    else
                        *(float *)rcv_data= *(float*)(send_data);

                    break;
                }
            case SMI_SHORT:
                {
                    if(chan->next_contrib!=chan->root_rank)
                    {
                        char * ptr=chan->net.data+(chan->packet_element_id_rcv)*2;
                        *(short *)rcv_data= *(short*)(ptr);
                    }
                    else
                        *(short *)rcv_data= *(short*)(send_data);

                    break;
                }
            case SMI_DOUBLE:
                {
                    if(chan->next_contrib!=chan->root_rank)
                    {
                        char * ptr=chan->net.data+(chan->packet_element_id_rcv)*8;
                        *(double *)rcv_data= *(double*)(ptr);
                    }
                    else
                        *(double *)rcv_data= *(double*)(send_data);
                    break;
                }
        }*/
        chan->processed_elements_root++;
        chan->packet_element_id_rcv++;
        if( chan->packet_element_id_rcv==chan->elements_per_packet)
            chan->packet_element_id_rcv=0;

        if(chan->processed_elements_root==message_size)
        {   //we finished the data that need to be received from this contributor, go to the next one
            chan->processed_elements_root=0;
            chan->next_contrib++;
            chan->packet_element_id_rcv=0;
        }
    }
    else
    {
        //Non root rank, pack the data and send it
        char *conv=(char*)send_data;
        const int message_size=chan->send_count;
        chan->processed_elements++;
        //copy the data
        //COPY_DATA_TO_NET_MESSAGE(chan,net,conv);
        char *data_snd=chan->net.data;
        switch(chan->data_type)
        {
            case SMI_CHAR:
                data_snd[chan->packet_element_id]=*conv;
                break;
            case SMI_SHORT:
            #pragma unroll
                for(char jj=0;jj<2;jj++)
                    data_snd[chan->packet_element_id*2+jj]=conv[jj];
                break;
            case SMI_INT:
            case SMI_FLOAT:
            #pragma unroll
                for(char jj=0;jj<4;jj++)
                    data_snd[chan->packet_element_id*4+jj]=conv[jj];
                break;
            case SMI_DOUBLE:
            #pragma unroll
                for(int jj=0;jj<8;jj++) //copy the data
                    data_snd[chan->packet_element_id*8+jj]=conv[jj];
                break;
        }
        chan->packet_element_id++;

        if(chan->packet_element_id==chan->elements_per_packet || chan->processed_elements==message_size)
        {
            SET_HEADER_NUM_ELEMS(chan->net.header,chan->packet_element_id);
            SET_HEADER_DST(chan->net.header,chan->root_rank);

            const char chan_gather_idx=gather_table[chan->port];
            write_channel_intel(gather_channels[chan_gather_idx],chan->net);
            //first one is a SMI_SYNCH, used to communicate to the support kernel that it has to wait for a "ready to receive" from the root
            SET_HEADER_OP(chan->net.header,SMI_GATHER);
            chan->packet_element_id=0;
        }
    }
}

#endif // GATHER_H

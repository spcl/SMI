#ifndef GATHER_H
#define GATHER_H

/**
  Gather collective API implementation
*/

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "data_types.h"
#include "header_message.h"
#include "operation_type.h"
#include "network_message.h"


channel SMI_Network_message channel_gather_send __attribute__((depth(2)));


typedef struct __attribute__((packed)) __attribute__((aligned(64))){
    SMI_Network_message net;         //buffered network message

    uint processed_elements_root;
    uint recv_count;              //given in number of data elements
    char port;
    char packet_element_id_rcv;     //used by the receivers
    char next_rcv;                  //the next receiver
    char my_rank;                   //These two are essentially the Communicator
    SMI_Network_message net_2;        //buffered network message

    char root_rank;
    char num_rank;
    uint send_count;              //given in number of data elements
    uint processed_elements;        //how many data elements we have sent/received
    char packet_element_id;         //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    char data_type;               //type of message
    char size_of_type;              //size of data type
    char elements_per_packet;       //number of data elements per packet
    bool beginning;
}SMI_GatherChannel;


SMI_GatherChannel SMI_Open_gather_channel(uint send_count,  uint recv_count, SMI_Datatype data_type, uint port, uint root, uint my_rank, uint num_ranks)
{
    SMI_GatherChannel chan;
    chan.port=(char)port;
    chan.send_count=send_count;
    chan.recv_count=recv_count;
    chan.data_type=data_type;
    chan.my_rank=(char)my_rank;
    chan.root_rank=(char)root;
    chan.num_rank=(char)num_ranks;
    chan.next_rcv=0;
    chan.beginning=true;
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
            //TODO add more data types
    }

    //setup header for the message

    SET_HEADER_SRC(chan.net.header,root);
    SET_HEADER_PORT(chan.net.header,chan.port);        //used by destination
    SET_HEADER_NUM_ELEMS(chan.net.header,0);    //at the beginning no data
    SET_HEADER_OP(chan.net_2.header,SMI_SYNCH);
    SET_HEADER_PORT(chan.net_2.header,chan.port); 
    SET_HEADER_DST(chan.net_2.header,chan.root_rank);
     
    

    chan.processed_elements=0;
    chan.processed_elements_root=0;
    chan.packet_element_id=0;
    chan.packet_element_id_rcv=0;

    return chan;
}



void SMI_Gather(SMI_GatherChannel *chan, volatile void* send_data, volatile void* rcv_data)
{
    //take here the pointers to send/recv data to avoid fake dependencies
    if(chan->my_rank==chan->root_rank)//I'm the root
    {
        //we can't offload this part to the kernel, by passing a network message
        //because it will replies to the root once every elem_per_packet cycles

        //the root is responsible for splitting the data in packets
        //and set the right receviver.
        //If the receiver is itself it has to set the rcv_data accordingly
        const uint message_size=chan->recv_count;

        if(chan->next_rcv!=chan->my_rank && chan->processed_elements_root==0 )//at the beginning we have to send the request
        {
            // SMI_Network_message request;
            SET_HEADER_OP(chan->net.header,SMI_SYNCH);
            SET_HEADER_DST(chan->net.header,chan->next_rcv);
            SET_HEADER_PORT(chan->net.header,chan->port);
            const char chan_idx_control=cks_control_table[chan->port];
            write_channel_intel(cks_control_channels[chan_idx_control],chan->net);
            // printf("**Root send request to the rank %d\n",chan->next_rcv);
        }

        //   mem_fence(CLK_CHANNEL_MEM_FENCE);
        //receive the data
        if(chan->packet_element_id_rcv==0 && chan->next_rcv!=chan->my_rank) {
            const char chan_idx_data=ckr_data_table[chan->port];
            chan->net=read_channel_intel(ckr_data_channels[chan_idx_data]);
            //printf("Root, received data\n");
        }

        char *data_rcv=chan->net.data;
        switch(chan->data_type)
        {
            case SMI_CHAR:
                {
                    if(chan->next_rcv!=chan->root_rank)
                    {
                        char * ptr=data_rcv;
                        *(char *)rcv_data= *(char*)(ptr);
                    }
                    else
                        *(char *)rcv_data= *(char*)(send_data);
                    break;
                }
            case SMI_INT:
                {
                    if(chan->next_rcv!=chan->root_rank)
                    {
                        char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
                        *(int *)rcv_data= *(int*)(ptr);
                    }
                    else
                        *(int *)rcv_data= *(int*)(send_data);
                    break;
                }
            case SMI_FLOAT:
                {
                    if(chan->next_rcv!=chan->root_rank)
                    {
                        char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
                        *(float *)rcv_data= *(float*)(ptr);
                    }
                    else
                        *(float *)rcv_data= *(float*)(send_data);

                    break;
                }
                /*case SMI_DOUBLE:
                {
                    char * ptr=data_rcv+(chan->packet_element_id_rcv)*8;
                    *(double *)data= *(double*)(ptr);
                    break;
                }*/
        }
        chan->processed_elements_root++;
        chan->packet_element_id_rcv++;                       //first increment and then use it: otherwise compiler detects Fmax problems
        if( chan->packet_element_id_rcv==chan->elements_per_packet)
            chan->packet_element_id_rcv=0;

        if(chan->processed_elements_root==message_size)
        {   //we finished the data that need to be sent to this rcvr
            chan->processed_elements_root=0;
            chan->next_rcv++;
            chan->packet_element_id_rcv=0;

        }

    }
    else
    {
        //pack the data and when needed send it
        char *conv=(char*)send_data;
        char *data_snd=chan->net_2.data;
        const uint message_size=chan->send_count;
        chan->processed_elements++;
        switch(chan->data_type)
        {
            case SMI_CHAR:
                data_snd[chan->packet_element_id]=*conv;
                break;
            case SMI_INT:
            case SMI_FLOAT:
            #pragma unroll
                for(char jj=0;jj<4;jj++) //copy the data
                    data_snd[chan->packet_element_id*4+jj]=conv[jj];
                break;
                /*case SMI_DOUBLE:
                #pragma unroll
                for(int jj=0;jj<8;jj++) //copy the data
                    data_snd[chan->packet_element_id*8+jj]=conv[jj];
            break;*/
        }
        chan->packet_element_id++;
        //split this in packets holding send_count elements
        if(chan->packet_element_id==chan->elements_per_packet || chan->processed_elements==message_size) //send it if packet is filled or we reached the message size
        {

            SET_HEADER_NUM_ELEMS(chan->net_2.header,chan->packet_element_id);
            const char chan_gather_idx=gather_table[chan->port];
            write_channel_intel(gather_channels[chan_gather_idx],chan->net_2);
            SET_HEADER_OP(chan->net_2.header,SMI_GATHER);
            chan->packet_element_id=0;
        }
    }
}

#if 0
//this kernel is executed by the non-roots
__kernel void kernel_gather(char num_rank)
{
    //receives the data from the application and
    //forwards it to the root only when the
    //request arrives
    SMI_Network_message mess;

    while(true)
    {

        mess=read_channel_intel(channel_gather_send);
        if(GET_HEADER_OP(mess.header)==SMI_SYNCH)
        {

            SMI_Network_message req=read_channel_intel(channels_from_ck_r[1]);
        }

        write_channel_intel(channels_to_ck_s[1],mess);

    }

}
#endif
#endif // GATHER_H

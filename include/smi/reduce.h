
#ifndef REDUCE_H
#define REDUCE_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "data_types.h"
#include "header_message.h"
#include "network_message.h"
#include "operation_type.h"

typedef enum{
    SMI_ADD = 0,
    SMI_MAX = 1,
    SMI_MIN = 2
}SMI_Op;


typedef struct __attribute__((packed)) __attribute__((aligned(64))){
    SMI_Network_message net;            //buffered network message
    char port;                          //Output channel for the bcast, used by the root
    char root_rank;
    char my_rank;                       //communicator infos
    char num_rank;
    uint message_size;                  //given in number of data elements
    uint processed_elements;            //how many data elements we have sent/received
    char packet_element_id;             //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    SMI_Datatype data_type;             //type of message
    char size_of_type;                  //size of data type
    char elements_per_packet;           //number of data elements per packet
    SMI_Network_message net_2;          //buffered network message (we need two of them to remove aliasing)
    char packet_element_id_rcv;         //used by the receivers
}SMI_RChannel;


SMI_RChannel SMI_Open_reduce_channel(uint count, SMI_Datatype data_type, uint port, uint root, uint my_rank, uint num_ranks)
{
    SMI_RChannel chan;
    //setup channel descriptor
    chan.message_size=count;
    chan.data_type=data_type;
    chan.port=(char)port;
    chan.my_rank=(char)my_rank;
    chan.root_rank=(char)root;
    chan.num_rank=(char)num_ranks;
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

    //setup header for the message
    SET_HEADER_DST(chan.net.header,root);
    SET_HEADER_SRC(chan.net.header,my_rank);
    SET_HEADER_PORT(chan.net.header,chan.port);        //used by destination
    SET_HEADER_NUM_ELEMS(chan.net.header,0);           //at the beginning no data
    chan.processed_elements=0;
    chan.packet_element_id=0;
    chan.packet_element_id_rcv=0;
    return chan;
}


void SMI_Reduce(SMI_RChannel *chan, volatile void* data_snd, volatile void* data_rcv)
{

    char *conv=(char*)data_snd;
    #pragma unroll
    for(int jj=0;jj<chan->size_of_type;jj++) //copy the data
        chan->net.data[jj]=conv[jj];

    //In this case we disabled network packetization: so we can just send the data as soon as we have it
    SET_HEADER_NUM_ELEMS(chan->net.header,1);

    if(chan->my_rank==chan->root_rank) //root
    {
        write_channel_intel(reduce_send_channels[0],chan->net);
        mem_fence(CLK_CHANNEL_MEM_FENCE);
        chan->net_2=read_channel_intel(reduce_recv_channels[0]);

        char * ptr=chan->net_2.data;
        switch(chan->data_type)
        {
            case(SMI_SHORT):
                *(short *)data_rcv= *(short*)(ptr);
                break;
            case(SMI_INT):
                *(int *)data_rcv= *(int*)(ptr);
                break;
            case (SMI_FLOAT):
                *(float *)data_rcv= *(float*)(ptr);
                break;
            case (SMI_DOUBLE):
                *(double *)data_rcv= *(double*)(ptr);
                break;
            case (SMI_CHAR):
                *(char *)data_rcv= *(char*)(ptr);
                break;
        }
    }
    else
    {

        //wait for credits
        const char chan_idx_control=ckr_control_table[chan->port];
        SMI_Network_message req=read_channel_intel(ckr_control_channels[chan_idx_control]);
        mem_fence(CLK_CHANNEL_MEM_FENCE);
        //then send the data
        //printf("NOn-root, received credits, send data\n");
        SET_HEADER_OP(chan->net.header,SMI_REDUCE);
        const char chan_idx_data=cks_data_table[chan->port];
        write_channel_intel(cks_data_channels[chan_idx_data],chan->net);
    }

}

#endif // REDUCE_H

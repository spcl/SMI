#ifndef SCATTER_H
#define SCATTER_H

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "data_types.h"
#include "header_message.h"
#include "operation_type.h"
#include "network_message.h"


channel SMI_Network_message channels_scatter_send __attribute__((depth(2))); //CODEGEN

typedef struct __attribute__((packed)) __attribute__((aligned(64))){
    SMI_Network_message net;         //buffered network message
    char port;                    //Output channel for the bcast, used by the root
    char root_rank;
    char my_rank;                   //These two are essentially the Communicator
    char num_rank;
    uint send_count;              //given in number of data elements
    uint recv_count;              //given in number of data elements
    uint processed_elements;        //how many data elements we have sent/received
    char packet_element_id;         //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    char data_type;               //type of message
    SMI_Network_message net_2;        //buffered network message
    char size_of_type;              //size of data type
    char elements_per_packet;       //number of data elements per packet
    bool beginning;
    char packet_element_id_rcv;     //used by the receivers
    char next_rcv;                  //the next receiver
}SMI_ScatterChannel;


SMI_ScatterChannel SMI_Open_scatter_channel(uint send_count,  uint recv_count, SMI_Datatype data_type, uint port, uint root, uint my_rank, uint num_ranks)
{
    SMI_ScatterChannel chan;
    //setup channel descriptor
    chan.send_count=send_count;
    chan.recv_count=recv_count;
    chan.data_type=data_type;
    chan.port=(char)port;
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
    }

    //setup header for the message
    if(my_rank!=root)
    {
        //this is set up to send a "ready to receive" to the root
        SET_HEADER_OP(chan.net.header,SMI_SYNCH);
        SET_HEADER_DST(chan.net.header,root);
        SET_HEADER_PORT(chan.net.header,chan.port);
        const char chan_idx_control=internal_to_cks_control_rt[chan.port];
        write_channel_intel(channels_cks_control[chan_idx_control],chan.net); 
        // printf("non-root rank %d, I've sent the request\n",chan->my_rank);
    }
    else
    {
        SET_HEADER_SRC(chan.net.header,root);
        SET_HEADER_PORT(chan.net.header,chan.port);         //used by destination
        SET_HEADER_NUM_ELEMS(chan.net.header,0);            //at the beginning no data
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
        const uint message_size=chan->send_count;
        chan->processed_elements++;
        switch(chan->data_type)
        {
            case SMI_CHAR:
                if(chan->next_rcv==chan->my_rank)
                    data_snd[0]=*conv;
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
            if(chan->beginning) 
            {
                SET_HEADER_OP(chan->net.header,SMI_SYNCH);
                chan->beginning=false;
            }
            else
                 SET_HEADER_OP(chan->net.header,SMI_SCATTER);
            if(chan->next_rcv!=chan->my_rank)
                write_channel_intel(channels_scatter_send,chan->net);
            chan->packet_element_id=0;
            if(chan->processed_elements==message_size)
            {   //we finished the data that need to be sent to this rcvr
                chan->processed_elements=0;
                chan->next_rcv++;
            }
        }
    }
    else //I have to receive
    {
        if(chan->packet_element_id_rcv==0)
        {
          //  const char chan_idx=internal_receiver_rt[chan->tag_in];
            const char chan_idx_data=internal_from_ckr_data_rt[chan->port];
            chan->net_2=read_channel_intel(channels_ckr_data[chan_idx_data]);
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


//temp here, then if it works we need to move it
//TODO: This receives the data only from the root
//But it doesn't know when this is a beginning of a new broadcast
//We can use
//- special operation SMI_REQUEST to indicate the beginning of a broadcast
//- then it will start receiving the SMI_REQUEST from all the involved ranks
//- only when these are received can start the broadcast as usual
__kernel void kernel_scatter(char num_rank)
{
    //decide whether we keep this argument or not
    //otherwise we have to decide where to put it
    bool external=true;
    char received_request=0; //how many ranks are ready to receive
    const char num_requests=num_rank-1;
    SMI_Network_message mess;

    while(true)
    {
        if(external) //read from the application
        {
            mess=read_channel_intel(channels_scatter_send);
            if(GET_HEADER_OP(mess.header)==SMI_SYNCH)
                received_request=num_requests;
            external=false;
        }
        else //handle the request
        {
            if(received_request!=0)
            {
                SMI_Network_message req=read_channel_intel(channels_ckr_control[0]); //CODEGEN
                received_request--;
            }
            else
            {
                //just push it to the network
                write_channel_intel(channels_cks_data[0],mess); //CODEGEN
                external=true;
            }
        }
    }

}


#endif // BCAST_H

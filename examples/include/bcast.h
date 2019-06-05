/** Bcast implementations for kmeans
 *
 *  - bcast float will use channels_to_ck_s[2], ckr[1]
 * -  bcast int will use channels_to_ck_s[5], ckr[3]


*/

#ifndef BCAST_H
#define BCAST_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "smi/data_types.h"
#include "smi/header_message.h"
#include "smi/network_message.h"

//temp here, then need to move

channel SMI_Network_message channel_scatter_send[2] __attribute__((depth(2))); //channel from root to bcast kernel


//align to 64 to remove aliasing
typedef struct __attribute__((packed)) __attribute__((aligned(64))){
    SMI_Network_message net;          //buffered network message
    char tag_out;                    //Output channel for the bcast, used by the root
    char tag_in;                     //Input channel for the bcast. These two must be properly code generated. Good luck
    char root_rank;
    char my_rank;                   //These two are essentially the Communicator
    char num_rank;
    uint message_size;              //given in number of data elements
    uint processed_elements;        //how many data elements we have sent/received
    char packet_element_id;         //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    SMI_Datatype data_type;               //type of message
    char size_of_type;              //size of data type
    char elements_per_packet;       //number of data elements per packet
    SMI_Network_message net_2;        //buffered network message
    char packet_element_id_rcv;     //used by the receivers
}SMI_BChannel;


SMI_BChannel SMI_Open_bcast_channel(uint count, SMI_Datatype data_type, char root, char my_rank, char num_ranks)
{
    SMI_BChannel chan;
    //setup channel descriptor
    chan.message_size=count;
    chan.data_type=data_type;
    chan.tag_in=0;
    chan.tag_out=0;
    chan.my_rank=my_rank;
    chan.root_rank=root;
    chan.num_rank=num_ranks;

    //float or int
    chan.size_of_type=4;
    chan.elements_per_packet=7;

    //setup header for the message
    SET_HEADER_DST(chan.net.header,0);
    //SET_HEADER_SRC(chan.net.header,my_rank);
    SET_HEADER_SRC(chan.net.header,root);
    SET_HEADER_TAG(chan.net.header,0);        //used by destination
    SET_HEADER_NUM_ELEMS(chan.net.header,0);  //at the beginning no data
    chan.processed_elements=0;
    chan.packet_element_id=0;
    chan.packet_element_id_rcv=0;
    return chan;
}


//float bcast
void SMI_Bcast_float(SMI_BChannel *chan, volatile void* data/*, volatile void* data_rcv*/)
{
    //take here the pointers to send/recv data to avoid fake dependencies

    if(chan->my_rank==chan->root_rank)//I'm the root
    {
        //char pack_elem_id_snd=chan->packet_element_id;
        char *conv=(char*)data;
        char *data_snd=chan->net.data;
        chan->processed_elements++;
        #pragma unroll
        for(int jj=0;jj<4;jj++) //copy the data
            data_snd[chan->packet_element_id*4+jj]=conv[jj];
        chan->packet_element_id++;
        if(chan->packet_element_id==7 || chan->processed_elements==chan->message_size) //send it if packet is filled or we reached the message size
        {

            SET_HEADER_NUM_ELEMS(chan->net.header,chan->packet_element_id);
            chan->packet_element_id=0;
            //offload to bcast kernel
            write_channel_intel(channel_scatter_send[0],chan->net);
        }
    }
    else //I have to receive
    {
        if(chan->packet_element_id_rcv==0)
        {
            //const char chan_idx=internal_receiver_rt[chan->tag_in];
            chan->net_2=read_channel_intel(channels_from_ck_r[1]); //TODO fix this
        }
        //char * ptr=chan->net_2.data+(chan->packet_element_id_rcv);
        char *data_rcv=chan->net_2.data;
        char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
        *(float *)data= *(float*)(ptr);
        chan->packet_element_id_rcv++;                       //first increment and then use it: otherwise compiler detects Fmax problems
        if( chan->packet_element_id_rcv==7)
             chan->packet_element_id_rcv=0;
        //mem_fence(CLK_CHANNEL_MEM_FENCE);
    }

}



//int bcast
void SMI_Bcast_int(SMI_BChannel *chan, volatile void* data/*, volatile void* data_rcv*/)
{
    //take here the pointers to send/recv data to avoid fake dependencies

    if(chan->my_rank==chan->root_rank)//I'm the root
    {
        //char pack_elem_id_snd=chan->packet_element_id;
        char *conv=(char*)data;
        char *data_snd=chan->net.data;
        chan->processed_elements++;
        #pragma unroll
        for(int jj=0;jj<4;jj++) //copy the data
            data_snd[chan->packet_element_id*4+jj]=conv[jj];
        chan->packet_element_id++;
        if(chan->packet_element_id==7 || chan->processed_elements==chan->message_size) //send it if packet is filled or we reached the message size
        {

            SET_HEADER_NUM_ELEMS(chan->net.header,chan->packet_element_id);
            chan->packet_element_id=0;
            //offload to bcast kernel
            write_channel_intel(channel_scatter_send[1],chan->net);
        }
    }
    else //I have to receive
    {
        if(chan->packet_element_id_rcv==0)
        {
            //const char chan_idx=internal_receiver_rt[chan->tag_in];
            chan->net_2=read_channel_intel(channels_from_ck_r[3]);
        }
        //char * ptr=chan->net_2.data+(chan->packet_element_id_rcv);
        char *data_rcv=chan->net_2.data;
        char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
        *(int *)data= *(int*)(ptr);
        chan->packet_element_id_rcv++;                       //first increment and then use it: otherwise compiler detects Fmax problems
        if( chan->packet_element_id_rcv==7)
             chan->packet_element_id_rcv=0;
        //mem_fence(CLK_CHANNEL_MEM_FENCE);
    }

}


__kernel void kernel_bcast_float(char num_rank)
{
    bool external=true;
    char rcv;
    char root;
    SMI_Network_message mess;
    SET_HEADER_TAG(mess.header,1);
//  /  printf("bcast kernel!\n");
    while(true)
    {
        if(external)
        {
            mess=read_channel_intel(channel_scatter_send[0]);
            rcv=0;
            external=false;
            root=GET_HEADER_SRC(mess.header);
            SET_HEADER_SRC(mess.header,root);
            SET_HEADER_TAG(mess.header,1);
        }

        if(rcv!=root) //it's not me
        {
            SET_HEADER_DST(mess.header,rcv);
            write_channel_intel(channels_to_ck_s[2],mess);
        }
        rcv++;
        if(rcv==num_rank)
        {
            external=true;
        }
    }

}



__kernel void kernel_bcast_int(char num_rank)
{
    bool external=true;
    char rcv;
    char root;
    SMI_Network_message mess;
//  /  printf("bcast kernel!\n");
    while(true)
    {
        if(external)
        {
            mess=read_channel_intel(channel_scatter_send[1]);
            rcv=0;
            external=false;
            root=GET_HEADER_SRC(mess.header);
            SET_HEADER_SRC(mess.header,root);
            SET_HEADER_TAG(mess.header,3); //TODO fix this
        }

        if(rcv!=root) //it's not me
        {
            SET_HEADER_DST(mess.header,rcv);
            write_channel_intel(channels_to_ck_s[5],mess);
        }
        rcv++;
        if(rcv==num_rank)
        {
            external=true;
        }
    }

}


#endif // BCAST_H

#ifndef BCAST_H
#define BCAST_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "data_types.h"
#include "header_message.h"
#include "network_message.h"

//temp here, then need to move

channel SMI_Network_message channel_bcast_send __attribute__((depth(2)));
//channel SMI_Network_message channel_bcast_recv __attribute__((depth(2))); //not used here, to be decided


/*
 * 101 implementation of BCast
    Problems:
        - it works with FLOATS only, because the compiler is not able to handle it more "dynamically"
        - problem in the pop part, with the reset of packet_od. For the moment being forced at 7 (it's float, there is no flush)
        - it consumes a lot of resources (dunno)
 *
 */
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
    SET_HEADER_DST(chan.net.header,0);
    //SET_HEADER_SRC(chan.net.header,my_rank);
    SET_HEADER_TAG(chan.net.header,0);        //used by destination
    SET_HEADER_NUM_ELEMS(chan.net.header,0);    //at the beginning no data
    chan.processed_elements=0;
    chan.packet_element_id=0;
    chan.packet_element_id_rcv=0;
    return chan;
}


/*
 * This Bcast is the fusion between a pop and a push
 * NOTE: this is a naive implementation
 */
void SMI_Bcast(SMI_BChannel *chan, volatile void* data, volatile void* data_rcv)
{
    if(chan->my_rank==chan->root_rank)//I'm the root
    {
        char *conv=(char*)data;
        const char chan_idx_out=internal_sender_rt[chan->tag_out];  //This should be properly code generated, good luck
        #pragma unroll
        for(int jj=0;jj<4;jj++) //copy the data
            chan->net.data[chan->packet_element_id*4+jj]=conv[jj];
        //chan->net.data[chan->packet_element_id]=*conv;

        chan->processed_elements++;
        chan->packet_element_id++;
        if(chan->packet_element_id==chan->elements_per_packet || chan->processed_elements==chan->message_size) //send it if packet is filled or we reached the message size
        {

            SET_HEADER_NUM_ELEMS(chan->net.header,chan->packet_element_id);
            SET_HEADER_SRC(chan->net.header,chan->my_rank);
            //send this packet to all the ranks
            //naive implementation
            /*for(int i=0;i<chan->num_rank;i++)
            {
                if(i!=chan->my_rank) //it's not me
                {
                    SET_HEADER_DST(chan->net.header,i);
                    write_channel_intel(channels_to_ck_s[0],chan->net);
                }
            }*/
            //offload to bcast kernel
            write_channel_intel(channel_bcast_send,chan->net);
            chan->packet_element_id=0;
        }

    }
    else //I have to receive
    {
        //chan->net=read_channel_intel(channels_from_ck_r[0]);
        //in this case we have to copy the data into the target variable
        if(chan->packet_element_id_rcv==0)
        {
            const char chan_idx=internal_receiver_rt[chan->tag_in];
            chan->net_2=read_channel_intel(channels_from_ck_r[/*chan_idx*/0]);
        }
        char * ptr=chan->net_2.data+(chan->packet_element_id_rcv)*4;
        //char * ptr=chan->net_2.data+(chan->packet_element_id_rcv);
        chan->packet_element_id_rcv++;                       //first increment and then use it: otherwise compiler detects Fmax problems
        //TODO: this prevents HyperFlex (try with a constant and you'll see)
        //I had to put this check, because otherwise II goes to 2
        //if we reached the number of elements in this packet get the next one from CK_R
        if(chan->packet_element_id_rcv==7)
            chan->packet_element_id_rcv=0;
       // chan->processed_elements++;                      //TODO: probably useless
        *(float *)data_rcv= *(float*)(ptr);
        //*(char *)data_rcv= *(ptr);
        //create data element
  /*      if(chan->data_type==SMI_INT)
            *(int *)data= *(int*)(ptr);
        if(chan->data_type==SMI_FLOAT)
            *(float *)data= *(float*)(ptr);
        if(chan->data_type==SMI_DOUBLE)
            *(double *)data= *(double*)(ptr);
*/
        //mem_fence(CLK_CHANNEL_MEM_FENCE);
    }

}


//temp here, then if it works we need to move it

__kernel void kernel_bcast(char num_rank)
{
    //decide whether we keep this argument or not
    //otherwise we have to decide where to put it
    bool external=true;
    char rcv;
    char root;
    SMI_Network_message mess;
    while(true)
    {
        if(external)
        {
            mess=read_channel_intel(channel_bcast_send);
            rcv=0;
            external=false;
            root=GET_HEADER_SRC(mess.header);
        }

        if(rcv!=root) //it's not me
        {
            SET_HEADER_DST(mess.header,rcv);
            write_channel_intel(channels_to_ck_s[0],mess);
        }
        rcv++;
        if(rcv==num_rank)
        {
            external=true;
            //rcv=0;
        }
    }

}


#endif // BCAST_H

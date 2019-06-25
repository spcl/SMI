#ifndef BCAST_H
#define BCAST_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "data_types.h"
#include "header_message.h"
#include "operation_type.h"
#include "network_message.h"

//TO BE CODEGEN: internal tables also for this one?
channel SMI_Network_message channel_bcast_send __attribute__((depth(2)));

//align to 64 to remove aliasing
typedef struct __attribute__((packed)) __attribute__((aligned(64))){
    SMI_Network_message net;          //buffered network message
    char port;                    //Output channel for the bcast, used by the root
    char root_rank;
    char my_rank;                   //These two are essentially the Communicator
    char num_rank;
    uint message_size;              //given in number of data elements
    uint processed_elements;        //how many data elements we have sent/received
    char packet_element_id;         //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    SMI_Datatype data_type;               //type of message
    char size_of_type;              //size of data type
    char elements_per_packet;       //number of data elements per packet
    bool beginning;
     SMI_Network_message net_2;        //buffered network message
    char packet_element_id_rcv;     //used by the receivers
}SMI_BChannel;


SMI_BChannel SMI_Open_bcast_channel(uint count, SMI_Datatype data_type, uint port, uint root,  uint my_rank, uint num_ranks)
{
    SMI_BChannel chan;
    //setup channel descriptor
    chan.message_size=count;
    chan.data_type=data_type;
    chan.port=(char)port;
    chan.my_rank=(char)my_rank;
    chan.root_rank=(char)root;
    chan.num_rank=num_ranks;
    chan.beginning=true;
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
            //TODO add more data types
    }


    if(my_rank!=root)
    {
        //this is set up to send a "ready to receive" to the root
        SET_HEADER_OP(chan.net_2.header,SMI_SYNCH);
        SET_HEADER_DST(chan.net_2.header,root);
        SET_HEADER_PORT(chan.net_2.header,chan.port);
    }
    else
    {
        SET_HEADER_OP(chan.net.header,SMI_BROADCAST);
        SET_HEADER_SRC(chan.net.header,root);
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
    //take here the pointers to send/recv data to avoid fake dependencies
    const char elem_per_packet=chan->elements_per_packet;

    if(chan->my_rank==chan->root_rank)//I'm the root
    {

        //char pack_elem_id_snd=chan->packet_element_id;
        char *conv=(char*)data;

        char *data_snd=chan->net.data;
        const uint message_size=chan->message_size;
        chan->processed_elements++;

      // const char chan_idx_out=internal_sender_rt[chan->tag_out];  //This should be properly code generated, good luck
        if(chan->data_type==SMI_CHAR)
            data_snd[chan->packet_element_id]=*conv;
        if(chan->data_type==SMI_INT)
            #pragma unroll
                for(int jj=0;jj<2;jj++) //copy the data
                    data_snd[chan->packet_element_id*2+jj]=conv[jj];
        if(chan->data_type==SMI_INT)
            #pragma unroll
                for(int jj=0;jj<4;jj++) //copy the data
                    data_snd[chan->packet_element_id*4+jj]=conv[jj];
        if(chan->data_type==SMI_FLOAT)
            #pragma unroll
                for(int jj=0;jj<4;jj++) //copy the data
                    data_snd[chan->packet_element_id*4+jj]=conv[jj];

      /*  if(chan->data_type==SMI_DOUBLE)
             #pragma unroll
                for(int jj=0;jj<8;jj++) //copy the data
                    data_snd[chan->packet_element_id*8+jj]=conv[jj];
*/
        /*const char data_size=chan->size_of_type;
        #pragma unroll
                for(int jj=0;jj<data_size;jj++) //copy the data
                    data_snd[chan->packet_element_id*data_size+jj]=conv[jj];*/
        /*switch(chan->data_type) //this must be code generated
        {
            case SMI_CHAR:
                data_snd[chan->packet_element_id]=*conv;
            break;
            case SMI_INT:
            case SMI_FLOAT:
                #pragma unroll
                for(int jj=0;jj<4;jj++) //copy the data
                    data_snd[chan->packet_element_id*4+jj]=conv[jj];
            break;
           case SMI_DOUBLE:
                #pragma unroll
                for(int jj=0;jj<8;jj++) //copy the data
                    data_snd[chan->packet_element_id*8+jj]=conv[jj];
            break;
        }*/


        //chan->net.data[chan->packet_element_id]=*conv;
        chan->packet_element_id++;
        //chan->packet_element_id++;
        if(chan->packet_element_id==elem_per_packet || chan->processed_elements==message_size) //send it if packet is filled or we reached the message size
        {

            SET_HEADER_NUM_ELEMS(chan->net.header,chan->packet_element_id);
            SET_HEADER_PORT(chan->net.header,1); //TODO fix this

            //offload to bcast kernel
            if(chan->beginning) //at the beginning we have to indicate
            {
                SET_HEADER_OP(chan->net.header,SMI_SYNCH);
                chan->beginning=false;
            }
            else
                 SET_HEADER_OP(chan->net.header,SMI_BROADCAST);

            write_channel_intel(channel_bcast_send,chan->net);
            chan->packet_element_id=0;
        }

    }
    else //I have to receive
    {
        if(chan->beginning)//at the beginning we have to send the request
        {
            const char chan_idx_control=internal_to_cks_control_rt[/*chan->port*/1];
//            SET_HEADER_OP(chan->net.header,SMI_REQUEST);
//            SET_HEADER_DST(chan->net.header,chan->root_rank);
//            SET_HEADER_TAG(chan->net.header,0);
            write_channel_intel(channels_cks_control[chan_idx_control],chan->net); //TODO to fix
            //printf("non-root rank, I've sent the request\n");
            chan->beginning=false;


        }
        mem_fence(CLK_CHANNEL_MEM_FENCE);
        //ATTENTION: Here we are using two different messages (net and net_2) to send the ack to the root and to receive the data
        //This is done to avoid the compiler to do wrong choices (in some cases, even if there is a clear dependency, the read from
        //channel has been moved before the write)

        //chan->net=read_channel_intel(channels_from_ck_r[0]);
        //in this case we have to copy the data into the target variable
        //char pack_elem_id_rcv=chan->packet_element_id_rcv;
        if(chan->packet_element_id_rcv==0 &&  !chan->beginning)
        {
            const char chan_idx_data=internal_from_ckr_data_rt[/*chan->port*/1];
            chan->net=read_channel_intel(channels_ckr_data[chan_idx_data]);
            //chan->net_2=read_channel_intel(channels_from_ck_r[chan_idx]);
            //printf("Non root, received data\n");
        }
        //char * ptr=chan->net_2.data+(chan->packet_element_id_rcv);
        char *data_rcv=chan->net.data;
         //char * ptr=chan->net_2.data+(chan->packet_element_id)*chan->size_of_type;
       if(chan->data_type==SMI_CHAR)
             {
                char * ptr=data_rcv;
                *(char *)data= *(char*)(ptr);
            }
        if(chan->data_type==SMI_INT)
            {
                 char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
                 *(int *)data= *(int*)(ptr);
            }
        if(chan->data_type==SMI_FLOAT)
        {
              char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
                 *(float *)data= *(float*)(ptr);
        }
        if(chan->data_type==SMI_SHORT)
        {
              char * ptr=data_rcv+(chan->packet_element_id_rcv)*2;
                 *(short *)data= *(short*)(ptr);
        }


       // char * ptr=data_rcv+(chan->packet_element_id_rcv)*4;
       // *(float *)data= *(float*)(ptr);
        //pack_elem_id_rcv++;
        chan->packet_element_id_rcv++;                       //first increment and then use it: otherwise compiler detects Fmax problems
        //TODO: this prevents HyperFlex (try with a constant and you'll see)
        //I had to put this check, because otherwise II goes to 2
        //if we reached the number of elements in this packet get the next one from CK_R
        if( chan->packet_element_id_rcv==elem_per_packet)
             chan->packet_element_id_rcv=0;
        //    chan->packet_element_id_rcv=0;
        //else
        //    chan->packet_element_id_rcv=pack_elem_id_rcv;



       // chan->processed_elements++;                      //TODO: probably useless
        //*(char *)data_rcv= *(ptr);
        //create data element

        //mem_fence(CLK_CHANNEL_MEM_FENCE);
    }



}


//CODEGEN: This must be code generated for each BCAST and must know the channels id (or the port) from which send data
//and receive control
//Also, the name of this should be unique, and must be known by the host program
__kernel void kernel_bcast(char num_rank)
{
    bool external=true;
    char rcv;
    char root;
    char received_request=0; //how many ranks are ready to receive
    const char num_requests=num_rank-1;
    SMI_Network_message mess;
    while(true)
    {
        if(external) //read from the application
        {
            mess=read_channel_intel(channel_bcast_send);
            if(GET_HEADER_OP(mess.header)==SMI_SYNCH)
                received_request=num_requests;
            rcv=0;
            external=false;
            root=GET_HEADER_SRC(mess.header);
        }
        else //handle the request
        {
            if(received_request!=0)
            {
                const char chan_idx_control=internal_from_ckr_control_rt[1]; //TO BE CODE GENERATED, 0 IS THE PORT
                SMI_Network_message req=read_channel_intel(channels_ckr_control[chan_idx_control]);      //TO BE CODE GENERATED
                received_request--;
            }
            else
            {
                if(rcv!=root) //it's not me
                {
                    SET_HEADER_DST(mess.header,rcv);
                    SET_HEADER_PORT(mess.header,1);                 //PORT
                    const char chan_idx_data=internal_to_cks_data_rt[1];
                    write_channel_intel(channels_cks_data[chan_idx_data],mess);
                }
                rcv++;
                external=(rcv==num_rank);
            }
        }
    }
}

#endif // BCAST_H

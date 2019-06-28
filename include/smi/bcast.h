#ifndef BCAST_H
#define BCAST_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "data_types.h"
#include "header_message.h"
#include "operation_type.h"
#include "network_message.h"


//align to 64 to remove aliasing
typedef struct __attribute__((packed)) __attribute__((aligned(64))){
    SMI_Network_message net;            //buffered network message
    char port;                          //Output channel for the bcast, used by the root
    char root_rank;
    char my_rank;                       //These two are essentially the Communicator
    char num_rank;
    uint message_size;                  //given in number of data elements
    uint processed_elements;            //how many data elements we have sent/received
    char packet_element_id;             //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    SMI_Datatype data_type;             //type of message
    SMI_Network_message net_2;          //buffered network message: used for the receiving side
    char size_of_type;                  //size of data type
    char elements_per_packet;           //number of data elements per packet
    char packet_element_id_rcv;         //used by the receivers
}SMI_BChannel;

//TODO: communicator


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
        SET_HEADER_OP(chan.net.header,SMI_SYNCH);
        SET_HEADER_DST(chan.net.header,root);
        SET_HEADER_PORT(chan.net.header,chan.port);
        const char chan_idx_control=internal_to_cks_control_rt[chan.port];
        //SET_HEADER_OP(chan->net.header,SMI_SYNCH);
        //SET_HEADER_DST(chan->net.header,chan->root_rank);
        //SET_HEADER_PORT(chan->net.header,chan->port);
        write_channel_intel(channels_cks_control[chan_idx_control],chan.net); //TODO to fix
        // printf("non-root rank %d, I've sent the request\n",chan->my_rank);
        //chan->beginning=false;
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



void SMI_Bcast(SMI_BChannel *chan, volatile void* data/*, volatile void*recv_data*/)
{
    const char elem_per_packet=chan->elements_per_packet;
    char *conv=(char*)data;

    if(chan->my_rank==chan->root_rank)//I'm the root
    {

        //char pack_elem_id_snd=chan->packet_element_id;
        char *conv=(char*)data;

        char *data_snd=chan->net.data;
        const uint message_size=chan->message_size;
        chan->processed_elements++;

        /*  #pragma unroll
                for(int jj=0;jj<4;jj++) //copy the data
                    data_snd[chan->packet_element_id*4+jj]=conv[jj];
          */   
   
        switch(chan->data_type) //this must be code generated
        {
              case SMI_CHAR:
                data_snd[chan->packet_element_id]=*conv;
                break;
            case SMI_SHORT:
                #pragma unroll
                for(int jj=0;jj<2;jj++) //copy the data
                    data_snd[chan->packet_element_id*2+jj]=conv[jj];
                break;
            case SMI_INT:
            case SMI_FLOAT:
            #pragma unroll
                for(int jj=0;jj<4;jj++) //copy the data
                    data_snd[chan->packet_element_id*4+jj]=conv[jj];
                break;
            /*    case SMI_DOUBLE:
                #pragma unroll
                for(int jj=0;jj<8;jj++) //copy the data
                    data_snd[chan->packet_element_id*8+jj]=conv[jj];
                break;
                */
        }
        chan->packet_element_id++;
        if(chan->packet_element_id==chan->elements_per_packet || chan->processed_elements==message_size) //send it if packet is filled or we reached the message size
        {

            SET_HEADER_NUM_ELEMS(chan->net.header,chan->packet_element_id);
            SET_HEADER_PORT(chan->net.header,chan->port); //TODO fix this
            chan->packet_element_id=0;
            const char chan_bcast_idx=internal_bcast_rt[chan->port];
            write_channel_intel(channels_bcast_send[chan_bcast_idx],chan->net);
           // printf("Root sending data\n");
            SET_HEADER_OP(chan->net.header,SMI_BROADCAST);
        }

    }
    else //I have to receive
    {
       /* if(chan->beginning)//at the beginning we have to send the request
        {
            const char chan_idx_control=internal_to_cks_control_rt[chan->port];
            write_channel_intel(channels_cks_control[chan_idx_control],chan->net); //TODO to fix
            // printf("non-root rank %d, I've sent the request\n",chan->my_rank);
            chan->beginning=false;

        }*/
      //  mem_fence(CLK_CHANNEL_MEM_FENCE);
        if(chan->packet_element_id_rcv==0 )
        {
            const char chan_idx_data=internal_from_ckr_data_rt[chan->port];
            chan->net_2=read_channel_intel(channels_ckr_data[chan_idx_data]);
          //  printf("Non-root received data\n");
        }
        char *data_rcv=chan->net_2.data;
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
        /*if(chan->data_type==SMI_DOUBLE)
        {
            char * ptr=data_rcv+(chan->packet_element_id_rcv)*8;
            *(double *)data= *(double*)(ptr);
        }*/

        chan->packet_element_id_rcv++;                       //first increment and then use it: otherwise compiler detects Fmax problems
        if( chan->packet_element_id_rcv==chan->elements_per_packet)
            chan->packet_element_id_rcv=0;
    }



}


#if 0
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
            if(GET_HEADER_OP(mess.header)==SMI_SYNCH)   //beginning of a broadcast, we have to wait for "ready to receive"
                received_request=num_requests;
            SET_HEADER_OP(mess.header,SMI_BROADCAST);
            rcv=0;
            external=false;
            root=GET_HEADER_SRC(mess.header);
        }
        else //handle the request
        {
            if(received_request!=0)
            {
                const char chan_idx_control=internal_from_ckr_control_rt[0]; //TO BE CODE GENERATED, 0 IS THE PORT
                SMI_Network_message req=read_channel_intel(channels_ckr_control[chan_idx_control]);
                received_request--;
            }
            else
            {
                if(rcv!=root) //it's not me
                {
                    SET_HEADER_DST(mess.header,rcv);
                    SET_HEADER_PORT(mess.header,0);                 // TO BE CODE GENERATED PORT
                    const char chan_idx_data=internal_to_cks_data_rt[0];
                    write_channel_intel(channels_cks_data[chan_idx_data],mess);
                }
                rcv++;
                external=(rcv==num_rank);
            }
        }
    }
}
#endif
#endif // BCAST_H


#if 0

#define SMI_Bcast(chan,data) ({ \
    if(chan.data_type==SMI_INT) \
        SMI_Bcast_int(&chan,&data); \
    else \
        SMI_Bcast_other(&chan,&data); \
})
#define SMI_Open_bcast_channel(COUNT, DTYPE,PORT,ROOT,MY_RANK,NUM_RANKS) ({ \
    SMI_BChannel chan; \
    chan.message_size=COUNT; \
    chan.data_type=DTYPE; \
    chan.port=(char)PORT; \
    chan.my_rank=(char)MY_RANK; \
    chan.root_rank=(char)ROOT; \
    chan.num_rank=NUM_RANKS; \
    chan.beginning=true; \
    switch(DTYPE) \
    { \
        case(SMI_SHORT): \
            chan.size_of_type=2; \
            chan.elements_per_packet=14; \
            break; \
        case(SMI_INT): \
            chan.size_of_type=4; \
            chan.elements_per_packet=7; \
            break; \
        case (SMI_FLOAT): \
            chan.size_of_type=4; \
            chan.elements_per_packet=7; \
            break; \
        case (SMI_DOUBLE): \
            chan.size_of_type=8; \
            chan.elements_per_packet=3; \
            break; \
        case (SMI_CHAR): \
            chan.size_of_type=1; \
            chan.elements_per_packet=28; \
            break; \
    } \
    if(my_rank!=root){ \
        SET_HEADER_OP(chan.net.header,SMI_SYNCH); \
        SET_HEADER_DST(chan.net.header,root); \
        SET_HEADER_PORT(chan.net.header,chan.port); \
        const char chan_idx_control=internal_to_cks_control_rt[chan.port];\
        write_channel_intel(channels_cks_control[chan_idx_control],chan.net); \
    } \
    else    { \
        SET_HEADER_OP(chan.net.header,SMI_BROADCAST); \
        SET_HEADER_SRC(chan.net.header,root); \
        SET_HEADER_PORT(chan.net.header,chan.port); \
        SET_HEADER_NUM_ELEMS(chan.net.header,0);    \
    } \
    chan.processed_elements=0; \
    chan.packet_element_id=0; \
    chan.packet_element_id_rcv=0; \
    chan; \
})

#endif

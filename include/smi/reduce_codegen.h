/**
  Reduce collective for kmeans

  Reduce float: will use channels_to_cks[0] and [1] to send and channels_from_ckr[0] to receive
  Reduce int: will use channels_to_cks[3] and [4] to send and channels_from_ckr[3] to receive

*/

#ifndef REDUCE_H
#define REDUCE_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "data_types.h"
#include "header_message.h"
#include "network_message.h"
#include "operation_type.h"

//TO CODEGEN

channel SMI_Network_message channel_reduce_send[1] __attribute__((depth(1)));
channel SMI_Network_message channel_reduce_recv[1] __attribute__((depth(1)));



//align to 64 to remove aliasing
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
    SMI_Network_message net_2;          //buffered network message
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


//FLOAT TAILORED
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
        write_channel_intel(channel_reduce_send[0],chan->net);
        mem_fence(CLK_CHANNEL_MEM_FENCE);
        chan->net_2=read_channel_intel(channel_reduce_recv[0]);

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
                *(double *)data_rcv= *(float*)(ptr);
                break;
            case (SMI_CHAR):
                *(char *)data_rcv= *(char*)(ptr);
                break;
        }
    }
    else
    {

        //wait for credits
        const char chan_idx_control=internal_from_ckr_control_rt[chan->port];
        SMI_Network_message req=read_channel_intel(channels_ckr_control[chan_idx_control]);
        mem_fence(CLK_CHANNEL_MEM_FENCE);
        //then send the data
        SET_HEADER_OP(chan->net.header,SMI_BROADCAST);//whatever
        const char chan_idx_data=internal_to_cks_data_rt[chan->port];
        write_channel_intel(channels_cks_data[chan_idx_data],chan->net);
    }

}




__constant int SHIFT_REG=4; //JAKUB: This depends on the data type and operation
//Meaningful only on the root side
__kernel void kernel_reduce(const char num_rank)
{
    SMI_Network_message mess;
    SMI_Network_message reduce;
    bool init=true;
    char sender_id=0;
    const char credits_flow_control=16; //apparently, this combination (credits, max ranks) is the max that we can support with II=1
    //reduced results, organized in shift register to mask latency
    float __attribute__((register)) reduce_result[credits_flow_control][SHIFT_REG+1];  //JAKUB: type must be code generated
    char data_recvd[credits_flow_control];
    bool send_credits=false;//true if (the root) has to send reduce request
    char credits=credits_flow_control; //the number of credits that I have
    char send_to=0;
    char /*__attribute__((register))*/ add_to[MAX_RANKS];   //for each rank tells to what element in the buffer we should add the received item
    for(int i=0;i<credits_flow_control;i++)
    {
        data_recvd[i]=0;
        #pragma unroll
        for(int j=0;j<SHIFT_REG+1;j++)
            reduce_result[i][j]=0;
    }

    for(int i=0;i<MAX_RANKS;i++)
        add_to[i]=0;
    char current_buffer_element=0;
    char add_to_root=0;
    char contiguos_reads=0;
    while(true)
    {
        bool valid=false;
        if(!send_credits)
        {
            switch(sender_id)
            {   //for the root, I have to receive from both sides
                case 0:
                    mess=read_channel_nb_intel(channel_reduce_send[0],&valid); //JAKUB: internal channel id needs to be code generated
                    break;
                case 1: //read from CK_R, can be done by the root and by the non-root
                    mess=read_channel_nb_intel(channels_ckr_data[0],&valid); //JAKUB: port number need to be code generated
                    break;
            }
            if(valid)
            {
                char a;
                if(sender_id==0)
                {
                    //received root contribution to the reduced result
                    //apply reduce
                    char * ptr=mess.data;
                    float data= *(float*)(ptr);             //JAKUB: data tyep needs be code generated
                    reduce_result[add_to_root][SHIFT_REG]=data+reduce_result[add_to_root][0];
                    #pragma unroll
                    for(int j = 0; j < SHIFT_REG; ++j)
                        reduce_result[add_to_root][j] = reduce_result[add_to_root][j + 1];

                    data_recvd[add_to_root]++;
                    a=add_to_root;
                    send_credits=init;      //the first reduce, we send this
                    init=false;
                    add_to_root++;
                    if(add_to_root==credits_flow_control)
                        add_to_root=0;

                }
                else
                {
                    //received contribution from a non-root rank, apply reduce operation
                    contiguos_reads++;
                    char * ptr=mess.data;
                    char rank=GET_HEADER_SRC(mess.header);
                    float data= *(float*)(ptr);                 //JAKUB: data tyep needs to be code generated
                    char addto=add_to[rank];
                    data_recvd[addto]++;
                    a=addto;
                    reduce_result[addto][SHIFT_REG]=data+reduce_result[addto][0];        //SMI_ADD
                    #pragma unroll
                    for(int j = 0; j < SHIFT_REG; ++j)
                        reduce_result[addto][j] = reduce_result[addto][j + 1];

                    addto++;
                    if(addto==credits_flow_control)
                        addto=0;
                    add_to[rank]=addto;

                }
                if(data_recvd[current_buffer_element]==num_rank)
                {   //We received all the contributions, we can send result to application
                    char *data_snd=reduce.data;
                    //Build reduced result
                    float res=0;                                //JAKUB: data type needs to be code generated
                    #pragma unroll
                    for(int i=0;i<SHIFT_REG;i++)
                        res+=reduce_result[current_buffer_element][i];
                    char *conv=(char*)(&res);
                    #pragma unroll
                    for(int jj=0;jj<4;jj++) //copy the data         //JAKUB: this depends on the size of data (so 4 for float and int, 2 for char,...)
                        data_snd[jj]=conv[jj];
                    write_channel_intel(channel_reduce_recv[0],reduce);
                    send_credits=true;
                    credits++;
                    data_recvd[current_buffer_element]=0;

                    #pragma unroll
                    for(int j=0;j<SHIFT_REG+1;j++)
                        reduce_result[current_buffer_element][j]=0;
                    current_buffer_element++;
                    if(current_buffer_element==credits_flow_control)
                        current_buffer_element=0;

                }
            }
            if(sender_id==0)
                sender_id=1;
            else
                if(!valid || contiguos_reads==READS_LIMIT)
                {
                    sender_id=0;
                    contiguos_reads=0;
                }
        }
        else
        {
            //send credits
            if(send_to!=GET_HEADER_DST(mess.header))
            {
                SET_HEADER_OP(reduce.header,SMI_SYNCH);
                SET_HEADER_PORT(reduce.header,0);   //JAKUB: port number to be code generated
                SET_HEADER_DST(reduce.header,send_to);
                write_channel_intel(channels_cks_control[0],reduce); //JAKUB: port number to be code generated
            }
            send_to++;
            if(send_to==num_rank)
            {
                send_to=0;
                credits--;
                send_credits=(credits!=0);
            }
        }
    }
}


#endif // REDUCE_H

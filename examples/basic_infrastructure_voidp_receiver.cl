/**
    This is the same as basic infrastructure, but with void pointers
    and wirte read that are now called push and pop (to avoid confusion with
    write/read_channel)

    In this case push and pop takes void *. Therefore in the channel
    descriptor we have also the identification of the type
*/

#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../kernels/datatypes/network_message.h"

typedef enum{
    INT,
    FLOAT
}data_t;

typedef struct __attribute__((packed)) __attribute__((aligned(32))){
    network_message_t net;          //buffered network message
    char sender_rank;
    char receiver_rank;
    char tag;
    uint message_size;              //given in number of data elements
    uint processed_elements;        //how many data elements we have sent/received
    uint packet_element_id;         //given a packet, the id of the element that we are currently processing (from 0 to the data elements per packet)
    data_t data_type;               //type of message
    operation_t op_type;            //type of operation
    char size_of_type;              //size of data type
    char elements_per_packet;       //number of data elements per packet
}chdesc_t;

//TODO: generalize this routing tables

__constant char sender_rt[2]={0,1};
__constant char receiver_rt[3]={0,1,2};

#if defined(EMULATION)
channel network_message_t io_out __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel")));
#else

channel network_message_t io_out __attribute__((depth(16)))
                    __attribute__((io("kernel_input_ch0")));

#endif
//kernel_input_ch0

//internal switching tables
//these maps endpoint -> channel_id
//for the ck_s: (src rank, tag) -> channel_id
//for ck_r: (dst rank,tag) -> channel_id
#if defined (THREE_SENDERS)
    channel network_message_t chan_from_ck_r[3] __attribute__((depth(16)));
#else
    channel network_message_t chan_from_ck_r[2] __attribute__((depth(16)));
#endif





/**
    Channel helpers
*/

chdesc_t open_channel(char my_rank, char pair_rank, char tag, uint message_size, data_t type, operation_t op_type)
{
    chdesc_t chan;
    //setup channel descriptor
    chan.tag=tag;
    chan.message_size=message_size;
    chan.data_type=type;
    chan.op_type=op_type;

    switch(type)
    {
        case(INT):
            chan.size_of_type=4;
            chan.elements_per_packet=6;
            break;
        case (FLOAT):
            chan.size_of_type=4;
            chan.elements_per_packet=6;
            break;
    }
    if(op_type == SEND)
    {
        //setup header for the message
        SET_HEADER_DST(chan.net.header,pair_rank);
        SET_HEADER_SRC(chan.net.header,my_rank);
        SET_HEADER_TAG(chan.net.header,tag);
        SET_HEADER_OP(chan.net.header,SEND);

        chan.sender_rank=my_rank;
        chan.receiver_rank=pair_rank;
        chan.processed_elements=0;
        chan.packet_element_id=0;
    }
    else
        if(op_type == RECEIVE)
        {
            chan.packet_element_id=chan.elements_per_packet; //data per packet
            chan.processed_elements=0;
            chan.sender_rank=pair_rank;
            chan.receiver_rank=my_rank;

            //no need here to setup the header of the message
        }


    return chan;
}


void pop(chdesc_t *chan, void *data)
{
    //when it stalls? or return wrong messages? when we read more than we have...?
    //in this case we have to copy the data into the target variable
    if(chan->packet_element_id==chan->elements_per_packet)
    {
        const char chan_idx=receiver_rt[chan->tag];
        chan->packet_element_id=0;
        chan->net=read_channel_intel(chan_from_ck_r[chan_idx]);
    }
    char * ptr=chan->net.data+(chan->packet_element_id)*4;
    chan->packet_element_id++;                       //first increment and then use it: otherwise compiler detects Fmax problems
    chan->processed_elements++;   //this could be used for some basic checks
    //create packet
    if(chan->data_type==INT)
        *(int *)data= *(int*)(ptr);
    if(chan->data_type==FLOAT)
        *(float *)data= *(float*)(ptr);
}

//__attribute__((max_global_work_dim(0)))
//__attribute__((autorun))
__kernel void CK_receiver()
{

    while(1)
    {
        network_message_t m=read_channel_intel(io_out);
        //forward it to the right unpacker
        //TODO change this accordingly
        switch(m.header.tag)
        {
            case 0:
                write_channel_intel(chan_from_ck_r[0],m);
                break;
            case 1:
                write_channel_intel(chan_from_ck_r[1],m);
                break;
            #if defined(THREE_SENDERS)
            case 2:
                write_channel_intel(chan_from_ck_r[2],m);
                break;
            #endif
        }

    }

}
__kernel void app_receiver_1(__global volatile char *mem, const int N)
{
    char check=1;
    int expected_0=0;
    //in questo caso riceviamo da due
    chdesc_t chan=open_channel(0,0,0,N,INT,RECEIVE);
    for(int i=0; i< N;i++)
    {
        int rcvd;
        pop(&chan,&rcvd);
        //printf("[RCV] received %d (expected %d)\n",mess.data,expected_0);
        check &= (rcvd==expected_0);
        expected_0+=2;
    }

    *mem=check;
    //printf("Receiver 1 finished\n");
}


__kernel void app_receiver_2(__global volatile char *mem, const int N)
{
    char check=1;
    float expected_0=1.1f;
    //in questo caso riceviamo da due
    chdesc_t chan=open_channel(1,0,1,N,FLOAT,RECEIVE);

    for(int i=0; i< N;i++)
    {
        float rcvd;
        pop(&chan,&rcvd);
        //printf("[RCV] received %f (expected %f)\n",mess.data,expected_0);
        check &= (rcvd==(expected_0+i));
        //expected_0=expected_0+1.0f;
    }

    *mem=check;
    //printf("Receiver 2 finished\n");
}


#if defined (THREE_SENDERS)
__kernel void app_receiver_3(__global volatile char *mem, const int N)
{
    char check=1;
    float expected_0=3.1f;
    //in questo caso riceviamo da due
    chdesc_t chan=open_channel(1,0,2,N,FLOAT,RECEIVE);

    for(int i=0; i< N;i++)
    {
        float rcvd;
        pop(&chan,&rcvd);
        check &= (rcvd==(expected_0+i));
        //expected_0=expected_0+1.0f;
    }

    *mem=check;
    //printf("Receiver 2 finished\n");
}
#endif


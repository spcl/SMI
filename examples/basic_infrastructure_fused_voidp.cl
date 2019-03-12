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
    data_t type;                    //type of message
    char size_of_type;              //size of data type
    char elements_per_packet;       //number of data elements per packet
}chdesc_t;

//TODO: generalize this routing tables

__constant char sender_rt[2]={0,1};
__constant char receiver_rt[2]={0,1};


//channel network_message_t io_out __attribute__((depth(16)));
channel network_message_t io_out __attribute__((depth(16)))
                    __attribute__((io("kernel_input_ch0")));
//kernel_input_ch0
channel network_message_t chan_to_ck_s[2] __attribute__((depth(16)));
channel network_message_t chan_from_ck_r[2] __attribute__((depth(16)));



/**
    Channel helpers
*/

chdesc_t open_channel(char my_rank, char receiver_rank, char tag, uint message_size, data_t type)
{
   // printf("Size of network message: %d, sizeof header: %d, sizeof enum %d\n",sizeof(network_message_t), sizeof(header_t),sizeof(operation_t));
    chdesc_t chan;
    //setup channel descriptor
    chan.sender_rank=my_rank;
    chan.receiver_rank=receiver_rank;
    chan.tag=tag;
    chan.message_size=message_size;
    chan.processed_elements=0;
    chan.packet_element_id=0;
    chan.type=type;

    //setup header
    SET_HEADER_DST(chan.net.header,receiver_rank);
    SET_HEADER_SRC(chan.net.header,my_rank);
    SET_HEADER_TAG(chan.net.header,tag);
    SET_HEADER_OP(chan.net.header,SEND);

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
    return chan;
}

//For the moment being it is separated: at the end it should exist some flag in the
//channel descriptor to indicate the channel type
//there is no matching
chdesc_t open_receiver_channel(char my_rank, char tag, uint message_size, data_t type)
{
    chdesc_t chan;
    chan.sender_rank=my_rank;
    chan.receiver_rank=my_rank;
    chan.tag=tag;
    chan.message_size=message_size;
    chan.processed_elements=0;
    chan.type=type;

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
    chan.packet_element_id=chan.elements_per_packet; //data per packet

    return chan;
}

void push(chdesc_t *chan, void* data)
{
    //TODO: data size and element per packet depends on channel type
    char *conv=(char*)data;
    const char chan_idx=sender_rt[chan->tag];
    #pragma unroll
    for(int jj=0;jj<chan->size_of_type;jj++) //data size
    {
        chan->net.data[chan->packet_element_id*4+jj]=conv[jj];
    }
    chan->processed_elements++;
    chan->packet_element_id++;
    //printf("Sent: %d (out of %d), data id: %d\n",chan->sent,chan->message_size,chan->data_id);
    if(chan->packet_element_id==chan->elements_per_packet || chan->processed_elements==chan->message_size) //send it if packet is filled or we reached the message size
    {
        chan->packet_element_id=0;
        write_channel_intel(chan_to_ck_s[chan_idx],chan->net);
    }

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
    if(chan->type==INT)
        *(int *)data= *(int*)(ptr);
    if(chan->type==FLOAT)
        *(float *)data= *(float*)(ptr);
}
#if 0

__kernel void app_sender_1(const int N)
{

    chdesc_t chan=open_channel(0,0,0,N,INT);
    for(int i=0;i<N;i++)
    {
        int data=i*2;
        push(&chan,&data);
    }
}

__kernel void app_sender_2(const int N)
{

    const float start=1.1f;
    chdesc_t chan=open_channel(0,1,1,N,FLOAT);

    for(int i=0;i<N;i++)
    {
        float data=i+start;
        push(&chan,&data);
        //network_message_t mess;
        // write_channel_intel(chan_to_ck_s[1],mess);

    }

}


__kernel void CK_sender()
{
    const uint num_sender=2;
    uint sender_id=0;
    bool valid=false;
    network_message_t mess;
    while(1)
    {
        //TODO: change this according to the number of channels that you have
        switch(sender_id)
        {
            case 0:
            mess=read_channel_nb_intel(chan_to_ck_s[0],&valid);
            break;
            case 1:
            mess=read_channel_nb_intel(chan_to_ck_s[1],&valid);
            break;
        }
        sender_id=(sender_id+1)%num_sender;
        if(valid)
            write_channel_intel(io_out,mess);

    }

}
#endif

__kernel void CK_receiver()
{

    while(1)
    {
        network_message_t m=read_channel_intel(io_out);
        //forward it to the right unpacker
        //TODO change this accordingly
        //printf("CK_R received message for %d\n",m.header.dst);
        switch(m.header.dst)
        {
            case 0:
                write_channel_intel(chan_from_ck_r[0],m);
                break;
            case 1:
                write_channel_intel(chan_from_ck_r[1],m);
                break;
        }

    }

}
__kernel void app_receiver_1(__global volatile char *mem, const int N)
{
    char check=1;
    int expected_0=0;
    //in questo caso riceviamo da due
    chdesc_t chan=open_receiver_channel(0,0,N,INT);
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
    chdesc_t chan=open_receiver_channel(0,1,N,FLOAT);

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
//#endif

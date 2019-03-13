/**
    In this example we have 1 sender (FPGA_0) and 2 receivers (FPGA_1)

                    -------> FPGA_1
                    |
        FPGA_0  ----
                    |
                    -------> FPGA_2

    The sender will be allocated on its own FPGA and communicates
    to the two receiver (a stream of ints)
    There will be two CK_S in the FPGA, that are connected each other.
    Beyond these connections, CK_S_0 that has two input channel from the
    sender and it is connected to the i/o channel directed to FPGA_1.
    CK_S_1 that is connected to FPGA_2
    Therefore, when CK_S_0 receives messages directed to FPGA_2, it will
    send them to CK_S_1

*/


#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../kernels/datatypes/network_message.h"
#include "../kernels/datatypes/channel_descriptor.h"

//internal routing tables
__constant char sender_rt[2]={0,1};
__constant char receiver_rt[2]={0,1};

//external routing tables: we will need one of this for each
//of the CK_S but also CK_R
//TODO: we have to reason on how we index this: rank start from zero
__constant char external_rt[3]={0,0,1}; //0 is external, 1 internal, the first ck_s in this case

//we have two network channels
channel network_message_t io_out[2] __attribute__((depth(16)));
//channel network_message_t io_out __attribute__((depth(16)))
//                    __attribute__((io("kernel_input_ch0")));
//kernel_input_ch0

//internal switching tables
//these maps endpoint -> channel_id
//for the ck_s: (src rank, tag) -> channel_id
//for ck_r: (dst rank,tag) -> channel_id

//these two channel goes from application sender to ck_s_0
channel network_message_t chan_to_ck_s_0[2] __attribute__((depth(16)));
//thene we have a channel between ck_s_0 and ck_s_1 (actually they should be two)
channel network_message_t interconnect_ck_s __attribute__((depth(16)));

//channel between ck_r and receiver (ok for now with the single implemetntation but then we have to move this
//and adjust the internal routing tables)
channel network_message_t chan_from_ck_r[2] __attribute__((depth(16)));


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
        //printf("Push invio a %d\n",chan_idx);
        write_channel_intel(chan_to_ck_s_0[chan_idx],chan->net);
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
    if(chan->data_type==INT)
        *(int *)data= *(int*)(ptr);
    if(chan->data_type==FLOAT)
        *(float *)data= *(float*)(ptr);
}

__kernel void app_sender(const int N)
{

    chdesc_t chan=open_channel(0,1,0,N,INT,SEND);
    chdesc_t chan2=open_channel(0,2,1,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
        push(&chan2,&data);

    }
}

__kernel void CK_sender_0()
{
    const uint num_sender=2;
    uint sender_id=0;
    bool valid=false;
    network_message_t mess;
    while(1)
    {
        //TODO: change this according to the number of channels that you have
        //in this case it receives from the two channel that arrive from app_sender
        switch(sender_id)
        {
            case 0:
            mess=read_channel_nb_intel(chan_to_ck_s_0[0],&valid);
            break;
            case 1:
            mess=read_channel_nb_intel(chan_to_ck_s_0[1],&valid);
            break;
        }
        if(valid)
        {
            //get the destination if internal of external
            char ext_rout=external_rt[GET_HEADER_DST(mess.header)];
            //printf("Header dst: %d rout: %d\n",GET_HEADER_DST(mess.header),ext_rout);
            //this is crossbar: but it is ok in this way
            switch(ext_rout)
            {
                case 0: //external
            //    printf("Sendo esterno\n");
                    write_channel_intel(io_out[0],mess);
                break;
                case 1: //to the other ck_s
          //      printf("Sendo interno\n");
                    write_channel_intel(interconnect_ck_s,mess);
                break;                    
            }
        }
        sender_id=(sender_id+1)%num_sender;

    }
}

__kernel void CK_sender_1()
{
    //this is simplified wrt to the other one, but in theory they are equal
    //TODO handle input from other CK_S in the swich_id: put it in another switch??
    const uint num_sender=2;
    uint sender_id=0;
    bool valid=false;
    network_message_t mess;
    while(1)
    {
        //TODO: change this according to the number of channels that you have
        //in this case it receives from the two channel that arrive from app_sender
        mess=read_channel_intel(interconnect_ck_s);
        //printf("Ricevuto messaggio\n");
        write_channel_intel(io_out[1],mess);
    }
}

__kernel void CK_receiver_0()
{
    //TODO: fix the rank-based switching

    while(1)
    {
        network_message_t m=read_channel_intel(io_out[0]);
        //forward it to the right unpacker
        //TODO change this accordingly
        //printf("CK_R received message for %d\n",m.header.dst);
        switch(m.header.dst)
        {
            case 1:
                write_channel_intel(chan_from_ck_r[0],m);
                break;
        }

    }
}

__kernel void CK_receiver_1()
{

    while(1)
    {
        network_message_t m=read_channel_intel(io_out[1]);
        //forward it to the right unpacker
        //TODO change this accordingly
       // printf("CK_R received message for %d\n",m.header.dst);
        switch(m.header.dst)
        {
            case 2:
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
    chdesc_t chan=open_channel(0,0,0,N,INT,RECEIVE);
    for(int i=0; i< N;i++)
    {
        int rcvd;
        pop(&chan,&rcvd);
        //printf("[RCV 1] received %d (expected %d)\n",rcvd,expected_0);
        check &= (rcvd==expected_0);
        expected_0++;
    }

    *mem=check;
    //printf("Receiver 1 finished\n");
}


__kernel void app_receiver_2(__global volatile char *mem, const int N)
{
    char check=1;
    int expected_0=0;
    //in questo caso riceviamo da due
    chdesc_t chan=open_channel(1,0,1,N,INT,RECEIVE);

    for(int i=0; i< N;i++)
    {
        int rcvd;
        pop(&chan,&rcvd);
        //printf("[RCV] received %f (expected %f)\n",mess.data,expected_0);
        //printf("[RCV 2] received %d (expected %d)\n",rcvd,expected_0);
        check &= (rcvd==(expected_0));
        expected_0++;
        //expected_0=expected_0+1.0f;
    }

    *mem=check;
    //printf("Receiver 2 finished\n");
}
//#endif






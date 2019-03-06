/**
    The example shows a basic communication infrastructure constituted by
    two application senders, two application receivers, packers, unpackers and CKs

    This version is obtained by fusing packing and unpacking directly in push/pop operations

    The code for the various entity is a copy-pasted-adapted version of the one
    contained in the kernels folder


    Everything is on the same FPGA. It can be tested on two FPGAs by using I/O channels

    Sender_1 sends integer to Receiver_1
    Sender_2 sends floats to Receiver_2


*/


#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../kernels/datatypes/network_message.h"



typedef struct __attribute__((packed)){
    char sender_rank;
    char receiver_rank;
    char tag;
    uint message_size;
    uint sent; //how many data we have sent
    network_message_t net;
    uint data_id;
}chdesc_t;




channel network_message_t io_out __attribute__((depth(16)));

channel network_message_t chan_to_ck_s[2] __attribute__((depth(16)));
channel network_message_t chan_from_ck_r[2] __attribute__((depth(16)));



/**
    Channel helpers
*/

chdesc_t open_channel(char my_rank, char receiver_rank, char tag, uint message_size)
{
    chdesc_t chan;
    chan.sender_rank=my_rank;
    chan.receiver_rank=receiver_rank;
    chan.tag=tag;
    chan.message_size=message_size;
    chan.net.header.dst=receiver_rank;
    chan.net.header.src=my_rank;
    chan.net.header.size=message_size;
    chan.sent=0;
    chan.data_id=0;
    return chan;
}

//For the moment being it is separated: at the end it should exist some flag in the
//channel descriptor to indicate the channel type
//there is no matching
chdesc_t open_receiver_channel(char my_rank, char tag, uint message_size)
{
    chdesc_t chan;
    chan.sender_rank=my_rank;
    chan.receiver_rank=my_rank;
    chan.tag=tag;
    chan.message_size=message_size;
    chan.data_id=6; //data per packet
    chan.sent=0;

    return chan;
}

void write_message_int(chdesc_t *chan, int data)
{

    char *conv=(char*)&data;
    for(int jj=0;jj<4;jj++) //data size
    {
        chan->net.data[chan->data_id*4+jj]=conv[jj];
    }
    chan->sent++;
    chan->data_id++;
    //printf("Sent: %d (out of %d), data id: %d\n",chan->sent,chan->message_size,chan->data_id);
    if(chan->data_id==6 || chan->sent==chan->message_size) //send it if packet is filled or we reached the message size
    {
        chan->data_id=0;
        write_channel_intel(chan_to_ck_s[0],chan->net);
    }

}

void write_message_float(chdesc_t *chan, float data)
{

    char *conv=(char*)&data;
    #pragma unroll
    for(int jj=0;jj<4;jj++)
    {
        chan->net.data[chan->data_id*4+jj]=conv[jj];
    }
    chan->sent++;
    chan->data_id++;
    if(chan->data_id==6 || chan->sent==chan->message_size) //send it if packet is filled or we reached the message size
    {
        chan->data_id=0;
        write_channel_intel(chan_to_ck_s[1],chan->net);
    }


}

int read_message_int(chdesc_t *chan)
{
    //when it stalls? or return wrong messages? when we read more than we have...?
    if(chan->data_id==6)
    {
        chan->data_id=0;
        chan->net=read_channel_intel(chan_from_ck_r[0]);
    }
    char * ptr=chan->net.data+(chan->data_id)*4;
    chan->data_id++;                       //first increment and then use it: otherwise compiler detects Fmax problems
    chan->sent++;   //this could be used for some basic checks
    //create packet
    int *rcv=(int*)(ptr);
    return rcv[0];
}

float read_message_float(chdesc_t *chan)
{
    //when it stalls? or return wrong messages? when we read more than we have...?
    if(chan->data_id==6)
    {
        chan->data_id=0;
        chan->net=read_channel_intel(chan_from_ck_r[1]);
    }
    char * ptr=chan->net.data+(chan->data_id)*4;
    chan->data_id++;                       //first increment and then use it: otherwise compiler detects Fmax problems
    chan->sent++;   //this could be used for some basic checks
    //create packet
    float *rcv=(float*)(ptr);
    //printf("Read 1 returns: %f\n",rcv[0]);

    return rcv[0];
}




__kernel void app_sender_1(const int N)
{

    chdesc_t chan=open_channel(0,0,0,N);
    for(int i=0;i<N;i++)
    {
        write_message_int(&chan,i*2);
    }
}

__kernel void app_sender_2(const int N)
{

    const float start=1.1f;
    chdesc_t chan=open_channel(0,1,1,N);

    for(int i=0;i<N;i++)
    {
        write_message_float(&chan,i+start);
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
    chdesc_t chan=open_receiver_channel(0,1,N);
    for(int i=0; i< N;i++)
    {
        int rcvd=read_message_int(&chan);
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
    chdesc_t chan=open_receiver_channel(0,1,N);

    for(int i=0; i< N;i++)
    {
        float rcvd=read_message_float(&chan);
        //printf("[RCV] received %f (expected %f)\n",mess.data,expected_0);
        check &= (rcvd==(expected_0+i));
        //expected_0=expected_0+1.0f;
    }

    *mem=check;
    //printf("Receiver 2 finished\n");
}

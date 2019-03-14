/**
    This is a dummy test, in which we try to understand
    how much a CK_Sender switch/case.
    This is useful also for the design choice "single ck" vs "a ck for each QSFP"

    In this case we evaluate the sender, in which we have an external routing
    table (random but defined at compile time).
    On the receiver side we just have 4 kernels that read from the different io_channels

    Resource and latency for a generic number of dummy receivers.
    The latency is the latency of the while loop.
    Everything has II=1 and Hyperflex Off on 18.1.1

    SENDERS can be Applications but also other CK_S if we have one CK_S per QSFP
    DSPs number is always 0
    #Sendr   FF      ALUT    RAM    MLAB    Lat
    1       1698	2059	7   6   39
    2       2874	2978	7   7   37
    4       4528	3256	7   7   37
    8       4528	3256	7   7   37
    12      5611	3522	7   7   37
    16      6663	3530	7   7   37
*/

#pragma OPENCL EXTENSION cl_intel_channels : enable


#include "../kernels/datatypes/network_message.h"
#include "../kernels/datatypes/channel_descriptor.h"
#define SENDERS 16
//internal routing tables
__constant char sender_rt[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

__constant char external_rt[16]={3,0,1,3,0,2,1,3,2,0,3,1,1,2,0,2}; //random

//we have 4 network channels
channel network_message_t io_out[4] __attribute__((depth(16)));


//these two channel goes from application sender to ck_s
channel network_message_t chan_to_ck_s[SENDERS] __attribute__((depth(16)));



void push(chdesc_t *chan, void* data)
{
    //TODO: data size and element per packet depends on channel type
    char *conv=(char*)data;
    const char chan_idx=sender_rt[chan->tag];
    #pragma unroll
    for(int jj=0;jj<chan->size_of_type;jj++) //data size
    {
        chan->net.data[chan->packet_element_id*chan->size_of_type+jj]=conv[jj];
    }
    chan->processed_elements++;
    chan->packet_element_id++;
    //printf("Sent: %d (out of %d), data id: %d\n",chan->sent,chan->message_size,chan->data_id);
    if(chan->packet_element_id==chan->elements_per_packet || chan->processed_elements==chan->message_size) //send it if packet is filled or we reached the message size
    {
        chan->packet_element_id=0;
        //printf("Push invio a %d\n",chan_idx);
        write_channel_intel(chan_to_ck_s[chan_idx],chan->net);
    }

}



__kernel void CK_sender()
{
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
                mess=read_channel_nb_intel(chan_to_ck_s[0],&valid);
            break;
#if (SENDERS >1)
            case 1:
                mess=read_channel_nb_intel(chan_to_ck_s[1],&valid);
            break;
#if (SENDERS >2)
            case 2:
                mess=read_channel_nb_intel(chan_to_ck_s[2],&valid);
            break;
#if (SENDERS >3)
            case 3:
                mess=read_channel_nb_intel(chan_to_ck_s[3],&valid);
            break;
#if (SENDERS >4)
            case 4:
                mess=read_channel_nb_intel(chan_to_ck_s[4],&valid);
            break;
#if (SENDERS >5)
            case 5:
                mess=read_channel_nb_intel(chan_to_ck_s[5],&valid);
            break;
#if (SENDERS >6)
            case 6:
                mess=read_channel_nb_intel(chan_to_ck_s[6],&valid);
            break;
#if (SENDERS >7)
            case 7:
                mess=read_channel_nb_intel(chan_to_ck_s[7],&valid);
            break;
#if (SENDERS >8)
            case 8:
                mess=read_channel_nb_intel(chan_to_ck_s[8],&valid);
            break;
#if (SENDERS >9)
            case 9:
                mess=read_channel_nb_intel(chan_to_ck_s[9],&valid);
            break;
#if (SENDERS >10)
            case 10:
                mess=read_channel_nb_intel(chan_to_ck_s[10],&valid);
            break;
#if (SENDERS >11)
            case 11:
                mess=read_channel_nb_intel(chan_to_ck_s[11],&valid);
            break;
#if (SENDERS >12)
            case 12:
                mess=read_channel_nb_intel(chan_to_ck_s[12],&valid);
            break;
#if (SENDERS >13)
            case 13:
                mess=read_channel_nb_intel(chan_to_ck_s[13],&valid);
            break;
#if (SENDERS >14)
            case 14:
                mess=read_channel_nb_intel(chan_to_ck_s[14],&valid);
            break;
#if (SENDERS >15)
            case 15:
                mess=read_channel_nb_intel(chan_to_ck_s[15],&valid);
            break;
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

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
                    write_channel_intel(io_out[0],mess);
                break;
                case 1:
                    write_channel_intel(io_out[1],mess);
                break;
                case 2:
                    write_channel_intel(io_out[2],mess);
                break;
                case 3:
                    write_channel_intel(io_out[3],mess);
                break;
            }
        }
        sender_id=sender_id+1;
        if(sender_id==SENDERS)
                sender_id=0;

    }
}


__kernel void app_sender0(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,0,0,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}

#if (SENDERS > 1)
__kernel void app_sender1(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,1,1,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}
#if (SENDERS > 2)
__kernel void app_sender2(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,3,2,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}

#if (SENDERS > 3)
__kernel void app_sender3(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,2,3,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}
#if (SENDERS > 4)
__kernel void app_sender4(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,1,4,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}

#if (SENDERS > 5)
__kernel void app_sender5(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,4,5,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}

#if (SENDERS > 6)
__kernel void app_sender6(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,5,6,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}

#if (SENDERS > 7)
__kernel void app_sender7(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,6,7,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}

#if (SENDERS > 8)
__kernel void app_sender8(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,7,8,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}

#if (SENDERS > 9)
__kernel void app_sender9(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,8,9,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}
#if (SENDERS > 10)
__kernel void app_sender10(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,9,10,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}
#if (SENDERS > 11)
__kernel void app_sender11(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,10,11,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}

#if (SENDERS > 12)
__kernel void app_sender12(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,11,12,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}

#if (SENDERS > 13)
__kernel void app_sender13(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,12,13,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}

#if (SENDERS > 14)
__kernel void app_sender14(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,0,14,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}
#if (SENDERS > 15)
__kernel void app_sender15(const int N)
{
    //here the desination rank is useless. We can have duplicates.
    //it is just used to route
    chdesc_t chan=open_channel(0,0,15,N,INT,SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        push(&chan,&data);
    }
}


#endif

#endif
#endif
#endif
#endif

#endif

#endif


#endif

#endif
#endif
#endif

#endif

#endif
#endif

#endif

//We have one receiver per IO channel
__kernel void receiver0()
{
    //in this case it just receives from io_out
    while(1)
        read_channel_intel(io_out[0]);
}

//We have one receiver per IO channel
__kernel void receiver1()
{
    //in this case it just receives from io_out
    while(1)
        read_channel_intel(io_out[1]);
}

//We have one receiver per IO channel
__kernel void receiver2()
{
    //in this case it just receives from io_out
    while(1)
        read_channel_intel(io_out[2]);
}

//We have one receiver per IO channel
__kernel void receiver3()
{
    //in this case it just receives from io_out
    while(1)
        read_channel_intel(io_out[3]);
}

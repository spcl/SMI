/**
    The example shows a basic communication infrastructure constituted by
    two application senders, two application receivers, packers, unpackers and CKs

    The code for the various entity is a copy-pasted-adapted version of the one
    contained in the kernels folder

    +------------+        +----------+                                                          +------------+          +--------------+
    |            |        |          |                                                          |            |          |              |
    |  Sender_1  +------->+  Packer  +----------------+                            +------------>  Unpacker  +--------->+  Receiver_1  |
    |            |        |          |                |                            |            |            |          |              |
    +------------+        +----------+                v                            |            +------------+          +--------------+
                                                  +---+----+                  +----+---+
                                                  |        |                  |        |
                                                  |  CK_S  +----------------->+  CK_R  |
                                                  |        |                  |        |
    +------------+        +----------+            +---+----+                  +----+---+        +------------+          +--------------+
    |            |        |          |                ^                            |            |            |          |              |
    |  Sender_2  +------->+  Packer  +----------------+                            +------------>  Unpacker  +--------->+  Receiver_2  |
    |            |        |          |                                                          |            |          |              |
    +------------+        +----------+                                                          +------------+          +--------------+


    Everything is on the same FPGA. It can be tested on two FPGAs by using I/O channels

    Sender_1 sends integer to Receiver_1
    Sender_2 sends floats to Receiver_2


*/


#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../kernels/datatypes/network_message.h"


//Message sent from applications to the
typedef struct __attribute__((packed)) {
   header_t header;
   int data;
}app_1_message_t;


typedef struct __attribute__((packed))  {
   header_t header;
   float data;
}app_2_message_t;


typedef struct __attribute__((packed)){
    char sender_rank;
    char receiver_rank;
    char tag;
    uint message_size;

}chdesc_t;




channel network_message_t io_out __attribute__((depth(16)));

channel network_message_t chan_to_ck_s[2] __attribute__((depth(16)));
channel app_1_message_t chan_to_packer_1 __attribute__((depth(16)));
channel app_2_message_t chan_to_packer_2 __attribute__((depth(16)));
channel app_1_message_t chan_from_unpacker_1 __attribute__((depth(16)));
channel app_2_message_t chan_from_unpacker_2 __attribute__((depth(16)));
channel network_message_t chan_from_ck_r[2] __attribute__((depth(16)));

#define DATA_TYPE_1 int
#define DATA_SIZE 4
#define DATA_PER_PACKET 6
#define DATA_TYPE_2 float
#define DATA_SIZE 4
#define DATA_PER_PACKET 6


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
    return chan;
}

void write_message_int(chdesc_t chan, int data)
{
    app_1_message_t mess;
    mess.header.size=chan.message_size;
    mess.header.src=chan.sender_rank;
    mess.header.dst=chan.receiver_rank;
    mess.data=data;
    write_channel_intel(chan_to_packer_1,mess);
}

void write_message_float(chdesc_t chan, float data)
{
    app_2_message_t mess;
    mess.header.size=chan.message_size;
    mess.header.src=chan.sender_rank;
    mess.header.dst=chan.receiver_rank;
    mess.data=data;
    write_channel_intel(chan_to_packer_2,mess);

}


__kernel void app_sender_1(const int N)
{

    chdesc_t chan=open_channel(0,0,0,N);
    for(int i=0;i<N;i++)
    {
        write_message_int(chan,i*2);
    }

}

__kernel void app_sender_2(const int N)
{

    const float start=1.1f;
    chdesc_t chan=open_channel(0,1,1,N);

    for(int i=0;i<N;i++)
    {
        write_message_float(chan,i+start);

    }

}


__kernel void packer_1()
{
    //TODO: exit conditions
    network_message_t mess;
    bool first=true;
    int size=1;
    int data_id=0; //current number of data in the network packet

    //NOTE: for the moment being it is assumed that the application message
    //report the (total) message size
    while(1)
    {

        app_1_message_t app=read_channel_intel(chan_to_packer_1);
        size--; //decrement is here to avoid Fmax problems
        if(first)
            size=app.header.size-1;

        //create the message
        //copy the data
        char *conv=(char*)&app.data;
        #pragma unroll
        for(int jj=0;jj<DATA_SIZE;jj++)
            mess.data[data_id*DATA_SIZE+jj]=conv[jj];
        mess.header=app.header;
        first=(size==0);

        if(data_id==DATA_PER_PACKET-1 || first) //send it if packet is filled or we reached the message size
        {
            data_id=0;
            write_channel_intel(chan_to_ck_s[0],mess);
        }
        else
            data_id++;
    }
}



__kernel void packer_2()
{
    //TODO: exit conditions
    network_message_t mess;
    bool first=true;
    int size=1;
    int data_id=0; //current number of data in the network packet

    //NOTE: for the moment being it is assumed that the application message
    //report the (total) message size
    while(1)
    {

        app_2_message_t app=read_channel_intel(chan_to_packer_2);
        size--; //decrement is here to avoid Fmax problems
        if(first)
            size=app.header.size-1;

        //create the message
        //copy the data
        char *conv=(char*)&app.data;
        #pragma unroll
        for(int jj=0;jj<DATA_SIZE;jj++)
            mess.data[data_id*DATA_SIZE+jj]=conv[jj];
        mess.header=app.header;
        first=(size==0);

        if(data_id==DATA_PER_PACKET-1 || first) //send it if packet is filled or we reached the message size
        {
            data_id=0;
            write_channel_intel(chan_to_ck_s[1],mess);
        }
        else
            data_id++;
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


__kernel void unpacker_1()
{
    //each received packet must be produced as a set of integers
    int size=-1;
    int it=DATA_PER_PACKET-1;

    int sent=0;
    bool first=true;

    network_message_t m;
    while(1)
    {
        if(it==DATA_PER_PACKET-1)
        {
            it=-1;
            m=read_channel_intel(chan_from_ck_r[0]);
            if(sent>size) //new message
            {
                size=m.header.size;
                sent=0;
            }
        }
        app_1_message_t app;
        char * ptr=m.data+(it)*DATA_SIZE;
        it++;                       //first increment and then use it: otherwise compiler detects Fmax problems
        sent++;
        //create packet
        app.header.size=m.header.size;
        app.header.src=m.header.src;
        app.header.dst=m.header.dst;
        DATA_TYPE_1 *rcv=(DATA_TYPE_1*)(ptr);

        app.data=rcv[1];
        if(sent<=size) //in this way we are wasting some iterations if the size is not a multiple but Hyperflex is on
            write_channel_intel(chan_from_unpacker_1,app);

    }
}

__kernel void unpacker_2()
{
    //each received packet must be produced as a set of integers
    int size=-1;
    int it=DATA_PER_PACKET-1;

    int sent=0;
    bool first=true;

    network_message_t m;
    while(1)
    {
        if(it==DATA_PER_PACKET-1)
        {
            it=-1;
            m=read_channel_intel(chan_from_ck_r[1]);
            if(sent>size) //new message
            {
                size=m.header.size;
                sent=0;
            }
        }
        app_2_message_t app;
        char * ptr=m.data+(it)*DATA_SIZE;
        it++;                       //first increment and then use it: otherwise compiler detects Fmax problems
        sent++;
        //create packet
        app.header.size=m.header.size;
        app.header.src=m.header.src;
        app.header.dst=m.header.dst;
        DATA_TYPE_2 *rcv=(DATA_TYPE_2*)(ptr);

        app.data=rcv[1];
        if(sent<=size) //in this way we are wasting some iterations if the size is not a multiple but Hyperflex is on
            write_channel_intel(chan_from_unpacker_2,app);

    }
}



__kernel void app_receiver_1(__global volatile char *mem, const int N)
{
    char check=1;
    int expected_0=0;
    //in questo caso riceviamo da due
    for(int i=0; i< N;i++)
    {
        app_1_message_t mess=read_channel_intel(chan_from_unpacker_1);
        //printf("[RCV] received %d (expected %d)\n",mess.data,expected_0);
        check &= (mess.data==expected_0);
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
    for(int i=0; i< N;i++)
    {
        app_2_message_t mess=read_channel_intel(chan_from_unpacker_2);
        //printf("[RCV] received %f (expected %f)\n",mess.data,expected_0);
        check &= (mess.data==(expected_0+i));
        //expected_0=expected_0+1.0f;
    }

    *mem=check;
    //printf("Receiver 2 finished\n");
}

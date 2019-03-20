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
    sender application and it is connected to the i/o channel directed to FPGA_1.
    CK_S_1 that is connected to FPGA_2
    Therefore, when CK_S_0 receives messages directed to FPGA_2, it will
    send them to CK_S_1

*/


#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../kernels/communications/channel_helpers.h"
#include "../kernels/communications/push.h"

#include "../kernels/communications/pop.h"

//#define EMULATION
#define ON_CHIP
//****This part should be code-generated****

#if defined(ON_CHIP)
//provisional, just for testing
channel SMI_NetworkMessage io_out[2] __attribute__((depth(16)));


#elif defined(EMULATION)
channel SMI_NetworkMessage io_out __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel")));
#else

channel SMI_NetworkMessage io_out __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch3")));
#endif
//internal routing tables
__constant char internal_sender_rt[2]={0,1};
__constant char internal_receiver_rt[2]={0,1};


//channel to CK_S
channel SMI_NetworkMessage channel_to_ck_s[2] __attribute__((depth(16)));

//channel from CK_R (provisional)
channel SMI_NetworkMessage channel_from_ck_r[2] __attribute__((depth(16)));

//inteconnection between CK_S (one of this two is actually never used)
channel SMI_NetworkMessage channel_interconnect_ck_s[2] __attribute__((depth(16)));



__kernel void CK_sender_0()
{
    //this must be loaded from DRAM
    //this maps ranks to channels_id
    //0->qsfp, 1-> the other CK_S
    char external_routing_table[3]={0,0,1}; //this receives packet to rank 1 e 2
    const char ck_id=0;
    /* CK_S accepts input data from Applications and
     * from other CK_Ss (and possibly one CK_R, future work).
     * It will read from these input_channel in RR, unsing non blocking read
     * The first id_s that are checked are related to application, then to other CK_S
     */
    //Number of sender is given by the number of application connected to this CK_S
    //and the number of CK_Ss pair
    const char num_sender=2+1;
    char sender_id=0;
    bool valid=false;
    SMI_NetworkMessage mess;
    while(1)
    {
        switch(sender_id)
        {
            case 0:
                mess=read_channel_nb_intel(channel_to_ck_s[0],&valid);
            break;
            case 1:
                mess=read_channel_nb_intel(channel_to_ck_s[1],&valid);
            break;
            case 2:
                mess=read_channel_nb_intel(channel_interconnect_ck_s[1],&valid);
            break;
        }

        if(valid)
        {
            //Here we don't distinguish between messages received from applications
            //or another CK_S
            //check the routing table
            char idx=external_routing_table[GET_HEADER_DST(mess.header)];
            switch(idx)
            {
                case 0:
                    write_channel_intel(io_out[0],mess);
                    break;
                case 1:
                    write_channel_intel(channel_interconnect_ck_s[0],mess);
                    break;

            }
            valid=false;
        }
        sender_id++;
        if(sender_id==num_sender)
                sender_id=0;

    }
}

__kernel void CK_sender_1()
{
    //this must be loaded from DRAM
    //this maps ranks to channels_id
    //0->qsfp, 1-> the other CK_S
    char external_routing_table[3]={100,100,0}; //this should receive packet to rank 1
    const char ck_id=1;
    //Ok this one is a special case in which we don't have application
    const char num_sender=1;
    char sender_id=0;
    bool valid=false;
    SMI_NetworkMessage mess;
    while(1)
    {
        switch(sender_id)
        {
            case 0:
                mess=read_channel_nb_intel(channel_interconnect_ck_s[0],&valid);
            break;
        }

        if(valid)
        {
            //check the routing table
            //in this case the packet should go outside
            char idx=external_routing_table[GET_HEADER_DST(mess.header)];
            switch(idx)
            {
                case 0:
                    write_channel_intel(io_out[1],mess);
                break;
                case 1: //this will never occur, but i've put it here to maintain this connection
                    write_channel_intel(channel_interconnect_ck_s[1],mess);
                break;

            }
            valid=false;
        }
        sender_id++;
        if(sender_id==num_sender)
                sender_id=0;

    }
}


__kernel void CK_receiver_0()
{

    while(1)
    {
#if defined(ON_CHIP)
           SMI_NetworkMessage m=read_channel_intel(io_out[0]);
#else
        SMI_NetworkMessage m=read_channel_intel(io_in);
#endif
        //forward it to the right application endpoint
        const char tag=GET_HEADER_TAG(m.header);
        const char chan_id=internal_receiver_rt[tag];
        //TODO generate this accordingly
        //NOTE: the explicit switch allows the compiler to generate the hardware
        switch(chan_id)
        {
            case 0:
                write_channel_intel(channel_from_ck_r[0],m);
            break;
        }

    }

}

__kernel void CK_receiver_1()
{

    while(1)
    {
#if defined(ON_CHIP)
           SMI_NetworkMessage m=read_channel_intel(io_out[1]);
#else
        SMI_NetworkMessage m=read_channel_intel(io_in);
#endif
        //forward it to the right application endpoint
        const char tag=GET_HEADER_TAG(m.header);
        const char chan_id=internal_receiver_rt[tag];
        //TODO generate this accordingly
        //NOTE: the explicit switch allows the compiler to generate the hardware
        switch(chan_id)
        {
            case 1:
                write_channel_intel(channel_from_ck_r[1],m);
            break;
        }

    }

}

//****End code-generated part ****



__kernel void app_sender(const int N)
{

    SMI_Channel chan=SMI_OpenChannel(0,1,0,N,SMI_INT,SMI_SEND);
    SMI_Channel chan2=SMI_OpenChannel(0,2,1,N,SMI_INT,SMI_SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        SMI_Push(&chan,&data);
        SMI_Push(&chan2,&data);
    }
}




__kernel void app_receiver_1(__global volatile char *mem, const int N)
{
    char check=1;
    int expected_0=0;
    //in questo caso riceviamo da due
    SMI_Channel chan=SMI_OpenChannel(0,0,0,N,SMI_INT,SMI_RECEIVE);
    for(int i=0; i< N;i++)
    {
        int rcvd;
        SMI_Pop(&chan,&rcvd);
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
    SMI_Channel chan=SMI_OpenChannel(1,0,1,N,SMI_INT,SMI_RECEIVE);

    for(int i=0; i< N;i++)
    {
        int rcvd;
        SMI_Pop(&chan,&rcvd);
        //printf("[RCV] received %f (expected %f)\n",mess.data,expected_0);
        //printf("[RCV 2] received %d (expected %d)\n",rcvd,expected_0);
        check &= (rcvd==(expected_0));
        expected_0++;
        //expected_0=expected_0+1.0f;
    }

    *mem=check;
    //printf("Receiver 2 finished\n");
}






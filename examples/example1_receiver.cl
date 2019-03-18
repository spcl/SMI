/**
   Example 1:
    The application is composed by two senders and two receivers.
    The two senders and the two receivers are allocated in two different FPGAs.

    app_sender_1 sends a message composed by N int (N passed as argument)
    app_sender_2 sends a message composed by N floats (N passed as argument)

    The receivers will check that the message is received in order.


    In this case we use a single QSFP: so we have only one CK_S.
    Connection between application endpoints to CK_S, internal_routing_tables, code
    for the CK should be code-generated. Here are manually generated and inserted
    in the first part of the code.

    To run the code in Emulation, compile with the EMULATION macro defined

*/

#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../kernels/communications/channel_helpers.h"
#include "../kernels/communications/pop.h"


//****This part should be code-generated****

#if defined(EMULATION)
channel SMI_NetworkMessage io_in __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel")));
#else

channel SMI_NetworkMessage io_in __attribute__((depth(16)))
                    __attribute__((io("kernel_input_ch3")));

#endif


//Internal routing table: maps tag -> chan_idx
__constant char internal_receiver_rt[2]={0,1};

//In this case there is no external routing table
channel SMI_NetworkMessage channel_from_ck_r[2] __attribute__((depth(16)));


__kernel void CK_receiver()
{

    while(1)
    {
        SMI_NetworkMessage m=read_channel_intel(io_in);
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
            case 1:
                write_channel_intel(channel_from_ck_r[1],m);
            break;
        }

    }

}

//****End code-generated part ****


__kernel void app_receiver_1(__global volatile char *mem, const int N)
{
    char check=1;
    int expected_0=0;
    SMI_Channel chan=SMI_OpenChannel(0,0,0,N,SMI_INT,SMI_RECEIVE);
    for(int i=0; i< N;i++)
    {
        int rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==expected_0);
        expected_0+=1;
    }

    *mem=check;
}


__kernel void app_receiver_2(__global volatile char *mem, const int N)
{
    char check=1;
    float expected_0=1.1f;
    SMI_Channel chan=SMI_OpenChannel(1,0,1,N,SMI_FLOAT,SMI_RECEIVE);

    for(int i=0; i< N;i++)
    {
        float rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(expected_0+i));
    }

    *mem=check;
}

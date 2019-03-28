/**
        DISCLAIMER: this must be updated with the new API 


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
#include "../kernels/communications/push.h"


//****This part should be code-generated****


#if defined(EMULATION)
channel SMI_NetworkMessage io_out __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel")));
#else

channel SMI_NetworkMessage io_out __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch3")));
#endif

//Internal routing table: maps tag -> chan_to_ck_s idx
__constant char internal_sender_rt[2]={0,1};

//In this case there is no external routing table
channel SMI_NetworkMessage channels_to_ck_s[2] __attribute__((depth(16)));



__kernel void CK_sender()
{
    const char num_sender=2;
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
        }

        if(valid)
        {
            write_channel_intel(io_out,mess);
            valid=false;
        }
        sender_id++;
        if(sender_id==num_sender)
                sender_id=0;

    }
}

//****End code-generated part ****


__kernel void app_sender_1(const int N)
{
    bool immediate=false;
    SMI_Channel chan=SMI_OpenChannel(0,0,0,N,SMI_INT,SMI_SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        //each 16 we send immediately
        if(i % 17 == 0)
        {
            immediate=true;
        }
        else
            immediate=false;
        SMI_PushI(&chan,&data,immediate);
    }
}

__kernel void app_sender_2(const int N)
{

    const float start=1.1f;
    SMI_Channel chan=SMI_OpenChannel(0,1,1,N,SMI_FLOAT,SMI_SEND);

    for(int i=0;i<N;i++)
    {
        float data=i+start;
        SMI_Push(&chan,&data);
    }

}






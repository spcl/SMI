/**
    In this example we have 1 sender and two receivers.
    Sender is allocated on FPGA0, receivers are allocate on FPGA1

            FPGA0   -------> FPGA1

    On FPGA1, receivers are allocated on two different CK_Rs.
    CK_R_0 is responsible for receiving data from the QSFP0 (which is where the
    connection with FPGA0 is linked) and is connected to APP1 and CK_R_1 (in/out).

    CK_R_1 is responsible for receiving data from the QSFP1 (no attached connection)
    CK_R_0 and is connected to APP2.

    Therefore messages directed to APP2 are received from CK_R_0 and eventually routed to CK_R_1

*/





#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../../kernels/communications/channel_helpers.h"
#include "../../kernels/communications/push.h"


//#define EMULATION
//#define ON_CHIP
//****This part should be code-generated****

#if defined(EMULATION)
channel SMI_NetworkMessage io_out_0 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_0")));

#else

channel SMI_NetworkMessage io_out_0 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch1")));
#endif
//internal routing tables
__constant char internal_sender_rt[2]={0,1};


//channel to CK_S
channel SMI_NetworkMessage channels_to_ck_s[2] __attribute__((depth(16)));


//inteconnection between CK_S (one of this two is actually never used)
// Here is not used: I don't even created the second CK_S



__kernel void CK_sender_0(__global volatile char *restrict rt, const char R)
{
    //in this case the routing table is really simple
    //it is just RANK0->QSFP
    char external_routing_table[256];
    for(int i=0;i<R;i++)
        external_routing_table[i]=rt[i];

    const char ck_id=0;
    /* CK_S accepts input data from Applications and
     * from other CK_Ss (and possibly one CK_R, future work).
     * It will read from these input_channel in RR, unsing non blocking read
     * The first id_s that are checked are related to application, then to other CK_S
     */
    //Number of sender is given by the number of application connected to this CK_S
    //and the number of CK_Ss pair
    const char num_sender=2; //in this case there are no other CK_S
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
            //Here we don't distinguish between messages received from applications
            //or another CK_S
            //check the routing table
            char idx=external_routing_table[GET_HEADER_DST(mess.header)];
            switch(idx)
            {
                case 0:
                    write_channel_intel(io_out_0,mess);
                    break;

            }
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

    SMI_Channel chan=SMI_OpenChannel(0,1,0,N,SMI_INT,SMI_SEND);
    for(int i=0;i<N;i++)
    {
        int data=i;
        SMI_Push(&chan,&data);
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

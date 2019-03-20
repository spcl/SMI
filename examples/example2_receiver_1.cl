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

#include "../kernels/communications/pop.h"

//#define EMULATION
//#define ON_CHIP
//****This part should be code-generated****

#if defined(ON_CHIP)
//provisional, just for testing
channel SMI_NetworkMessage io_out[2] __attribute__((depth(16)));


#elif defined(EMULATION)
channel SMI_NetworkMessage io_in_0 __attribute__((depth(16)))
                    __attribute__((io("emulatedChannel_0")));
#else

channel SMI_NetworkMessage io_in_0 __attribute__((depth(16)))
                    __attribute__((io("kernel_output_ch0")));

#endif
//internal routing tables
__constant char internal_receiver_rt[2]={0,1};



//channel from CK_R (provisional)
channel SMI_NetworkMessage channel_from_ck_r[1] __attribute__((depth(16)));





//Attention: CK_R are slightly different between the two receivers (because
// they are connected to different input channels)
__kernel void CK_receiver()
{

    while(1)
    {
#if defined(ON_CHIP)
           SMI_NetworkMessage m=read_channel_intel(io_out[0]);
#else
        SMI_NetworkMessage m=read_channel_intel(io_in_0);
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

//****End code-generated part ****






__kernel void app_receiver(__global volatile char *mem, const int N)
{
    char check=1;
    int expected_0=0;
    //in questo caso riceviamo da due
    SMI_Channel chan=SMI_OpenChannel(1,0,0,N,SMI_INT,SMI_RECEIVE);
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






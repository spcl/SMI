/**
    Latency microbenchmark

    The benchmark is constituted by two ranks:
    - rank 0 sends data
    - rank 1 receives it

    Each one waits for the counterpart.
    This round-trip is done N times so the latency is the total time
    spent/2N

    Please note: the presence of the fence is necessary otherwise it can change the order of
    the push/pop
*/
#pragma OPENCL EXTENSION cl_intel_channels : enable

//For this benchmark we need two different programs,
//but both of them use the same number/type of ports
#include "latency_routing/smi-device-0.h"



__kernel void app(const int N, char dest_rank,SMI_Comm comm)
{
    int to_send;
    for(int i=0;i<N;i++)
    {

        SMI_Channel chan_send=SMI_Open_send_channel(1,SMI_INT,dest_rank,0,comm);
        SMI_Push(&chan_send,&to_send);
        mem_fence(CLK_CHANNEL_MEM_FENCE);

        SMI_Channel chan_receive=SMI_Open_receive_channel(1,SMI_INT,dest_rank,0,comm);
        SMI_Pop(&chan_receive,&to_send);
        to_send++;
        mem_fence(CLK_CHANNEL_MEM_FENCE);
    }
}

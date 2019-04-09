/**
    Latency microbenchmark

    The benchmark is constituted by two ranks:
    - rank 0 sends data
    - rank 1 receives it

    Each one waits for the counterpart.
    This round-trip is done N times so the latency is the total time
    spent/2N

    For the moment being we impose a dependency in the loop (we send what we receive)
    I don't know if it is necessary or not

    Important: you need to use the FENCE
*/
#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "smi_latency.h"



__kernel void app(const int N)
{
    int to_send;
    for(int i=0;i<N;i++)
    {

        SMI_Channel chan_send=SMI_Open_send_channel(1,SMI_INT,0,0);
        SMI_Channel chan_receive=SMI_Open_receive_channel(1,SMI_INT,0,0);
        SMI_Pop(&chan_receive,&to_send);
        mem_fence(CLK_CHANNEL_MEM_FENCE);
        to_send++;
        SMI_Push(&chan_send,&to_send);
        mem_fence(CLK_CHANNEL_MEM_FENCE);

    }
}

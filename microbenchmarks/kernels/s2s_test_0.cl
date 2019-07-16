#pragma OPENCL EXTENSION cl_intel_channels : enable

//For this benchmark we need two different programs,
//but both of them use the same number/type of ports
#define BUFFER_SIZE 256
#include <smi/push.h>
#include <smi/pop.h>

__kernel void app(const int N, SMI_Comm comm)
{
    int to_send;
    for(int i=0;i<N;i++)
    {
        SMI_Channel chan_send=SMI_Open_send_channel(1,SMI_INT,0,0,comm);
        SMI_Channel chan_receive=SMI_Open_receive_channel(1,SMI_INT,0,0,comm);
        SMI_Pop(&chan_receive,&to_send);
        mem_fence(CLK_CHANNEL_MEM_FENCE);
        to_send++;
        SMI_Push(&chan_send,&to_send);
        mem_fence(CLK_CHANNEL_MEM_FENCE);
    }
}

//The same as the other one but in this case we
//use a different polling schema in the CKs

#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "scaling_benchmark/smi_rank0.h"



__kernel void app_0(const int N, const char dst)
{
    for(int i=0;i<N;i++)
    {
        SMI_Channel chan_send1=SMI_Open_send_channel(1,SMI_INT,dst,0);
        SMI_Push(&chan_send1,&i);
    }
}

__kernel void app_1(const int N)
{
    //dummy, just used for the sake of compiling
    for(int i=0;i<N;i++)
    {
        SMI_Channel chan_send2=SMI_Open_send_channel(1,SMI_INT,1,01);
        SMI_Push(&chan_send2,&i);
    }
}

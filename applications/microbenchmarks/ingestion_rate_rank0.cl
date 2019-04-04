#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "smi_ingestion_rate_rank0.h"



__kernel void app_0(const int N)
{
    for(int i=0;i<N;i++)
    {
        SMI_Channel chan_send1=SMI_Open_send_channel(1,SMI_INT,1,0);
        SMI_Push(&chan_send1,&i);
        SMI_Channel chan_send2=SMI_Open_send_channel(1,SMI_INT,1,1);
        SMI_Push(&chan_send2,&i);
    }
}

//The same as the other one but in this case we
//use a different polling schema in the CKs

#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "scaling_benchmark/smi_rank0.h"



__kernel void app_0(const int N)
{
    int rcv;
    for(int i=0;i<N;i++)
    {
        SMI_Channel chan_receive=SMI_Open_receive_channel(1,SMI_INT,0,0);
        SMI_Pop(&chan_receive,&rcv);
        if(rcv!=i)
            printf("App_1, what a shame I was expecting %d and instead I received %d\n",i,rcv);
    }
}

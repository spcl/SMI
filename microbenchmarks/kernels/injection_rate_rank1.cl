/*
	Injection rate benchmark. Rank 1 (or, more in general, the receiver)
  receives the data through a transient channel with message size 1.
*/

#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "injection_rate_routing/smi-device-1.h"



__kernel void app(const int N,SMI_Comm comm)
{
    int rcv;
    for(int i=0;i<N;i++)
    {
        SMI_Channel chan_receive=SMI_Open_receive_channel(1,SMI_INT,0,0,comm);
        SMI_Pop(&chan_receive,&rcv);
    }
}

/*
	Injection rate benchmark. Rank 0 sends data using a
	transient channel with message size 1 to a receiving rank
	whose rank id is specied as parameter
*/

#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "injection_rate_codegen/smi-device-0.h"



__kernel void app(const int N, const char dst, SMI_Comm comm)
{
    for(int i=0;i<N;i++)
    {
        SMI_Channel chan_send=SMI_Open_send_channel(1,SMI_INT,dst,0,comm);
        SMI_Push(&chan_send,&i);
    }
}

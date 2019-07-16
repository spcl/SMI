/**
	Broadcast benchmark. A sequeuence of number is broadcasted.
	Non-root ranks check whether the received number is correct
*/

#pragma OPENCL EXTENSION cl_intel_channels : enable
#include "smi-generated-device.cl"
#include <smi/bcast.h>

__kernel void app(__global char* mem, const int N, char root,SMI_Comm comm)
{
    char check=1;
    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_FLOAT,0, root,comm);
    //SMI_BChannel  chan= SMI_Open_bcast_channel(N, SMI_FLOAT,0, root,comm);
    for(int i=0;i<N;i++)
    {

        float to_comm=i;
        SMI_Bcast(&chan,&to_comm);

           check &= (to_comm==i);
    }
    *mem=check;
}

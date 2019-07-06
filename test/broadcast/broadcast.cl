/**
	Broadcast test. A sequeuence of number is broadcasted.
	Non-root ranks check whether the received number is correct
*/

#pragma OPENCL EXTENSION cl_intel_channels : enable
#include "broadcast_routing/smi-device-0.h"



__kernel void test_int(__global char* mem, const int N, char root,SMI_Comm comm)
{
    char check=1;
    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_INT,0, root,comm);
    for(int i=0;i<N;i++)
    {

        int to_comm=i;
        SMI_Bcast(&chan,&to_comm);
        check &= (to_comm==i);
    }
    *mem=check;
}

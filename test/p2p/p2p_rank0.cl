/**
    P2P test. Rank 0 sends a stream of data
*/
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "p2p_routing/smi-device-0.h"

__kernel void app(const int N, const char dest_rank, const SMI_Comm comm)
{

    SMI_Channel chan=SMI_Open_send_channel(N,SMI_INT,dest_rank,0,comm);
    for(int i=0;i<N;i++)
    {
       int send=i;
       SMI_Push(&chan,&send);
    }
}


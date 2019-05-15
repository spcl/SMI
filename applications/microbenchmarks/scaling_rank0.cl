/**
    Scaling benchmark: we want to evaluate the bandwdith
    achieved between two ranks. The FPGA are connected in a chain
    so we can decide the distance at which they are

    RANK 0 is the source of the data
*/
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "scaling_benchmark/smi_rank0.h"

__kernel void app(const int N, const char dest_rank)
{
    SMI_Channel chan=SMI_Open_send_channel(N,SMI_DOUBLE,dest_rank,0);
    const double start=0.1f;
    const int outer_loop_limit=N/3;
    for(int i=0;i<outer_loop_limit;i++)
    {
        #pragma unroll
        for(int j=0;j<3;j++)
        {
            double send=start+i*3+j;
            SMI_Push(&chan,&send);
        }
    }
}

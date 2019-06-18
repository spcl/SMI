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
//    printf("Size of: %d",sizeof(SMI_Channel));
    SMI_Channel chan=SMI_Open_send_channel(N,SMI_DOUBLE,dest_rank,0);
    const double start=0.1f;
    for(int i=0;i<N;i++)
    {
        double send=start+i;
        SMI_Push(&chan,&send);
       // printf("[APP 0] sent %d\n",i);
    }
}

__kernel void app_1(const int N, const char dest_rank)
{
    SMI_Channel chan=SMI_Open_send_channel(N,SMI_DOUBLE,dest_rank,1);
    const double start=0.1f;
    for(int i=0;i<N;i++)
    {
        double send=start+i;
        SMI_Push(&chan,&send);
        //printf("[APP 1] sent %d\n",i);
    }
}


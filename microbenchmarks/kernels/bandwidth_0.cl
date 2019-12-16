/**
    Scaling benchmark: we want to evaluate the bandwdith
    achieved between two ranks. The FPGA are connected in a chain
    so we can decide the distance at which they are

    RANK 0 is the source of the data
*/

#include <smi.h>

__kernel void app(const int N, const char dest_rank, const SMI_Comm comm)
{

    SMI_Channel chan=SMI_Open_send_channel_ad(N, SMI_DOUBLE, dest_rank, 0, comm, 2048);
    const double start=0.1f;
    for(int i=0;i<N;i++)
    {
        double send=start+i;
       SMI_Push(&chan,&send);
    }
}

__kernel void app_1(const int N, const char dest_rank, const SMI_Comm comm)
{

    SMI_Channel chan=SMI_Open_send_channel_ad(N,SMI_DOUBLE,dest_rank,1, comm, 2048);
    const double start=0.1f;
    for(int i=0;i<N;i++)
    {
        double send=start+i;
        SMI_Push(&chan,&send);
    }
}


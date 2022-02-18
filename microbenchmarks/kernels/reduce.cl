/**
    Reduce benchmark: each ranks sends it's contribution (its rank id).
    The root checks that the result is correct
*/

#pragma OPENCL EXTENSION cl_intel_channels : enable
#include <smi.h>

#include "smi_generated_device.cl"

__kernel void app(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    float exp=(num_ranks*(num_ranks+1))/2;
    char check=1;

    SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel(N, SMI_FLOAT, SMI_ADD, 0,root,comm);
    for(int i=0;i<N;i++)
    {
        float to_comm, to_rcv=0;
        to_comm=my_rank+1;
        SMI_Reduce(&rchan_float,&to_comm, &to_rcv);
        if(my_rank==root)
            check &= (to_rcv==exp);
    }
    *mem=check;
}



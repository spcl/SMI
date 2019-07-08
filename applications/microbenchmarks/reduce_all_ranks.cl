#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "reduce_codegen/smi-device-0.h"

__kernel void app(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    float exp=(num_ranks*(num_ranks+1))/2;
    //float exp=num_ranks;
    char check=1;

    SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel(N, SMI_FLOAT, SMI_ADD, 0,root,comm);
    for(int i=0;i<N;i++)
    {
        float to_comm, to_rcv=0;
        to_comm=my_rank+1;
        SMI_Reduce(&rchan_float,&to_comm, &to_rcv);
        if(my_rank==root)
        {
            check &= (to_rcv==exp);
           // if(to_rcv!=exp)
             //   printf("Reduced %.0f, I was expecting %.0f\n",to_rcv,exp);
        }
    }
    *mem=check;
}



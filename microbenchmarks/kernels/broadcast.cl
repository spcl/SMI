#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "broadcast_routing/smi-device-0.h"


__kernel void app(__global char* mem, const int N, char root,char my_rank, char num_ranks/*, __global int * restrict  data*/)
{
    char check=1;

    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_FLOAT,0, root,my_rank,num_ranks);
    for(int i=0;i<data;i++)
    {

        int to_comm=i;//count_updated[i];
        SMI_Bcast(&chan,&to_comm);

           check &= (to_comm==i);
    }

    *mem=check;

}

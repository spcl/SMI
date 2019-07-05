/**
  Scatter benchmarks. The root scatters N*NUM_RANKS elements
  accross all the ranks. Cheks is performed on each rank
*/
#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "scatter_routing/smi-device-0.h"



__kernel void app(const int N, char root,__global char* mem, SMI_Comm comm)
{

    SMI_ScatterChannel  __attribute__((register)) chan= SMI_Open_scatter_channel(N,N, SMI_INT, 0,root,comm);
    char check=1;
    int num_ranks=SMI_Comm_size(comm);
    char my_rank=SMI_Comm_rank(comm);
    const int loop_bound=(my_rank==root)?N*num_ranks:N;
    const int to_rcv_start=my_rank*N;
    for(int i=0;i<loop_bound;i++)
    {
        int to_send, to_rcv;
        if(my_rank==root)
            to_send=i;

        SMI_Scatter(&chan,&to_send, &to_rcv);
        if(my_rank!=root && to_rcv_start+i!=to_rcv)
          check=0;
    }
    *mem=check;
}

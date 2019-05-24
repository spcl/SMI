
#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../host/k_means_routing/smi.h"

/**
  Each rank will sends its rank value +1 for reduction.
  The root store the reduced values in DRAM
  Then the root will broadcast the reduced value
*/

__kernel void app(const int iterations, const int N, const int M, char root,char my_rank, char num_ranks, __global volatile float *reduced_float,  __global volatile int *reduced_int, __global volatile char *mem)
{
    float exp=(num_ranks*(num_ranks+1))/2;
    int exp_int=(num_ranks*(num_ranks+1))/2;
    char check=1;
    for(int it=0;it<iterations;it++)
    {
        SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel(N, SMI_FLOAT, root,my_rank,num_ranks);
        for(int i=0;i<N;i++)
        {
            float to_comm, to_rcv=0;
            to_comm=my_rank+1;
            SMI_Reduce_float(&rchan_float,&to_comm, &to_rcv);
            if(my_rank==root)
                check &= (to_rcv==exp);
            if(my_rank==root)
                reduced_float[i]=to_rcv;

        }


        SMI_BChannel  __attribute__((register)) bchan_float= SMI_Open_bcast_channel(N, SMI_FLOAT, root,my_rank,num_ranks);
        for(int i=0;i<M;i++)
        {
            float bcast_data;
            if(my_rank==root)
                bcast_data=reduced_float[i];
            SMI_Bcast_float(&bchan_float,&bcast_data);
            reduced_float[i]=bcast_data;
            if(my_rank!=root) //check
                check &= (bcast_data==exp);

        }


        SMI_RChannel  __attribute__((register)) rchan_int= SMI_Open_reduce_channel(M, SMI_INT, root,my_rank,num_ranks);
        for(int i=0;i<M;i++)
        {
            int to_comm, to_rcv;
            to_comm=my_rank+1;
            SMI_Reduce_int(&rchan_int,&to_comm, &to_rcv);
            if(my_rank==root)
                check &= (to_rcv==exp_int);

            if(my_rank==root)
                reduced_int[i]=to_rcv;


        }

        SMI_BChannel  __attribute__((register)) bchan_int= SMI_Open_bcast_channel(M, SMI_INT, root,my_rank,num_ranks);
        for(int i=0;i<N;i++)
        {
            int bcast_data;
            if(my_rank==root)
                bcast_data=reduced_int[i];
            SMI_Bcast_int(&bchan_int,&bcast_data);
            reduced_int[i]=bcast_data;
            if(my_rank!=root) //check
                check &= (bcast_data==exp_int);
        }

    }
    *mem=check;
}

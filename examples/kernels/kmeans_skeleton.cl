
#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../host/k_means_routing/smi.h"

/**
  Each rank will sends its rank value +1 for reduction.
  The root store the reduced values in DRAM
  Then the root will broadcast the reduced value
*/

__kernel void app(const int iterations, const int N, char root,char my_rank, char num_ranks, __global volatile float *reduced, __global volatile char *mem)
{
    float exp=(num_ranks*(num_ranks+1))/2;
    char check=1;
    //printf("Iteration: %d, N: %d\n",iterations,N);
    for(int it=0;it<iterations;it++)
    {
        SMI_RChannel  __attribute__((register)) rchan= SMI_Open_reduce_channel(N, SMI_FLOAT, root,my_rank,num_ranks);
        for(int i=0;i<N;i++)
        {
            float to_comm, to_rcv=0;
            to_comm=my_rank+1;
            SMI_Reduce(&rchan,&to_comm, &to_rcv);
            if(my_rank==root)
                check &= (to_rcv==exp);
       //     if(my_rank==root && to_rcv!= exp)
         //       printf("!!!!!!!!!!!!!! Rank %d received %.0f while I was expecting %.0f\n",my_rank,to_rcv,exp);
            if(my_rank==root)
                reduced[i]=to_rcv;

            //broadcast
//            printf("Kernel %d performing the bcast")
//            SMI_Bcast(&bchan,&to_rcv);
//            if(my_rank!=root && to_rcv!= exp)
//                printf("!!!!!!!!!!!!!! Rank non root %d received %.0f while I was expecting %.0f\n",my_rank,to_rcv,exp);
//            else
//                printf("Rank non root %d received %.0f\n",my_rank,to_rcv);

        }

        //if(check==1)
          //  printf("Result is ok\n");

        SMI_BChannel  __attribute__((register)) bchan= SMI_Open_bcast_channel(N, SMI_FLOAT, root,my_rank,num_ranks);
        for(int i=0;i<N;i++)
        {
            float bcast_data;
            if(my_rank==root)
                bcast_data=reduced[i];
            SMI_Bcast(&bchan,&bcast_data);
            reduced[i]=bcast_data;
            if(my_rank!=root) //check
                check &= (bcast_data==exp);
//            if(my_rank!=root && bcast_data!= exp)
  //             printf("!!!!!!!!!!!!!! Rank non root %d received %.0f while I was expecting %.0f\n",my_rank,bcast_data,exp);

        }
        //if(check==1)
          //  printf("Result is ok\n");
    }
    *mem=check;
}

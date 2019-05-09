#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "smi_reduce_2.h"



__kernel void app(const int N, char root,char my_rank, char num_ranks, __global volatile char *mem/*, const int it*/)
{
    char exp=(num_ranks*(num_ranks+1))/2/*+ it*(num_ranks)*/;
    char check=1;
    for(int i=0;i<N;i++)
    {
        SMI_RChannel  __attribute__((register)) chan= SMI_Open_reduce_channel(1, SMI_INT, root,my_rank,num_ranks);
        int to_comm, to_rcv=0;
        to_comm=my_rank+1/*+it*/;

        SMI_Reduce(&chan,&to_comm, &to_rcv);
        if(my_rank==root)
            check &= (to_rcv==exp);
      //  printf("Rank %d reduced perfomed (%d out of %d)\n",my_rank,i,N);
       /* if(my_rank==root)
            printf("Root received: %d\n",to_rcv);*/
      //  if(my_rank==root && to_rcv!= exp)
       //     printf("!!!!!!!!!!!!!! Rank %d received %d while I was expecting %d\n",my_rank,to_rcv,exp);

    }
    *mem=check;
}

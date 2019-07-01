#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "gather_codegen/smi-device-0.h"



__kernel void app(const int N, char root,char my_rank, char num_ranks, __global char *mem)
{
    SMI_GatherChannel  __attribute__((register)) chan= SMI_Open_gather_channel(N,N, SMI_INT,0, root,my_rank,num_ranks);
    const int loop_bound=(my_rank==root)?N*num_ranks:N;
    int to_send=my_rank*N;
    char check=1;
    for(int i=0;i<loop_bound;i++)
    {
        int to_rcv;
        SMI_Gather(&chan,&to_send, &to_rcv);
        to_send++;
       // printf("Rank %d sent %d out of %d\n",my_rank,i,loop_bound);
        if(my_rank==root)
            check&=(to_rcv==i);
       // if(my_rank==root && to_rcv!=i)
        //   printf("Root received %d while I was expecting %d\n",to_rcv,i);
       // printf("Rank %d sent %.0f\n",my_rank,to_comm);
     //   acc+=to_comm;

        //if(my_rank!=root && to_rcv!=my_rank*N+i)
    }
    *mem=check;
}

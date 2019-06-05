#pragma OPENCL EXTENSION cl_intel_channels : enable

//#include "smi_broadcast.h"
#include "scatter/smi.h"



__kernel void app(const int N, char root,char my_rank, char num_ranks)
{
    SMI_ScatterChannel  __attribute__((register)) chan= SMI_Open_scatter_channel(N,N, SMI_INT, root,my_rank,num_ranks);
    printf("Sizeof %d\n",sizeof(SMI_ScatterChannel));
   // printf("Rank: %d, i have to send: %d\n",my_rank,N);
  //  for(int j=0;j<num_ranks;j++)
   // {
    const int loop_bound=(my_rank==root)?N*num_ranks:N;
    for(int i=0;i<loop_bound;i++)
    {
        int to_send, to_rcv;
        if(my_rank==root)
        {
            to_send=i;
        }

        SMI_Scatter(&chan,&to_send, &to_rcv);


       // printf("Rank %d sent %.0f\n",my_rank,to_comm);
     //   acc+=to_comm;

        //if(my_rank!=root && to_rcv!=my_rank*N+i)
          //  printf("Rank %d received %d while I was expecting %d\n",my_rank,to_rcv,my_rank*N+i);
    }
//    }
}

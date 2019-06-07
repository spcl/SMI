#pragma OPENCL EXTENSION cl_intel_channels : enable

//#include "smi_broadcast.h"
#include "broadcast_routing_2tags/smi.h"



__kernel void app(const int N, char root,char my_rank, char num_ranks)
{
    SMI_BChannel /* __attribute__((register))*/ chan= SMI_Open_bcast_channel(N, SMI_DOUBLE, root,my_rank,num_ranks);

   // printf("Rank: %d, i have to send: %d\n",my_rank,N);

    for(int i=0;i<N;i++)
    {
        float to_comm;
        if(my_rank==root)
            to_comm=i;
        SMI_Bcast(&chan,&to_comm);
       // printf("Rank %d sent %.0f\n",my_rank,to_comm);
     //   acc+=to_comm;

       // if(my_rank!=root && i!=to_comm)
         //     printf("Rank %d received %.0f while I was expecting %d\n",my_rank,to_comm,i);
    }
}

#pragma OPENCL EXTENSION cl_intel_channels : enable

//#include "smi_broadcast.h"
#include "broadcast/smi-0.h"



__kernel void app(__global char* mem, const int N, char root,char my_rank, char num_ranks)
{
    char check=1;
    //fictituos
    SMI_Channel rchan=SMI_Open_receive_channel(N,SMI_DOUBLE,0,0);
    SMI_Channel schan=SMI_Open_send_channel(N,SMI_DOUBLE,0,0);

    double data;
    SMI_Push(&schan,&data);
    SMI_Pop(&rchan,&data);


    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_FLOAT,1, root,my_rank,num_ranks);
   // printf("Rank: %d, i have to send: %d\n",my_rank,N);
    for(int i=0;i<N;i++)
    {

        float to_comm;
        if(my_rank==root)
            to_comm=i;
        SMI_Bcast(&chan,&to_comm);
       // printf("Rank %d sent %.0f\n",my_rank,to_comm);
     //   acc+=to_comm;

       if(my_rank!=root)
            check &= (to_comm==i);
         //     printf("Rank %d received %.0f while I was expecting %d\n",my_rank,to_comm,i);
    }
    *mem=check;

}

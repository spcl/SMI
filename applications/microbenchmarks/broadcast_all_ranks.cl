#pragma OPENCL EXTENSION cl_intel_channels : enable

//#include "smi_broadcast.h"
#include "broadcast/smi-0.h"



__kernel void app(__global char* mem, const int N, char root,char my_rank, char num_ranks/*, __global int * restrict  data*/)
{
    char check=1;
    //fictituos
    SMI_Channel rchan=SMI_Open_receive_channel(N,SMI_DOUBLE,0,0);
    SMI_Channel schan=SMI_Open_send_channel(N,SMI_DOUBLE,0,0);

    /*double dataa=N;
    SMI_Push(&schan,&dataa);
    SMI_Pop(&rchan,&dataa);


    int count_updated[32];
    for(int i=0;i<32;i++)
        count_updated[i]=data[i];
    int count_reduced[32];
*/


    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_INT,0, root,my_rank,num_ranks);
   // printf("Rank: %d, i have to send: %d\n",my_rank,N);
    for(int i=0;i<N;i++)
    {

        /*int to_comm;
        if(my_rank==root)
            to_comm=i;*/
        int to_comm=i;//count_updated[i];
        SMI_Bcast(&chan,&to_comm);
        //count_reduced[i]=to_comm;
       // printf("Rank %d sent %.0f\n",my_rank,to_comm);
     //   acc+=to_comm;

           //check &= (to_comm==i);
         //     printf("Rank %d received %.0f while I was expecting %d\n",my_rank,to_comm,i);
    }
    *mem=check;

}

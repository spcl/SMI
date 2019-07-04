#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "reduce_codegen/smi-device-0.h"

__kernel void app(const int N, char root,char my_rank, char num_ranks, __global volatile char *mem)
{
    //double exp=(num_ranks*(num_ranks+1))/2;
    int exp=0;
    char check=1;
//    SMI_Channel chans=SMI_Open_send_channel(N,SMI_INT,1,0);
//    SMI_Channel chanr=SMI_Open_receive_channel(N,SMI_INT,0,0);
//    int data=N;
//    SMI_Push(&chans,&data);
//    SMI_Pop(&chanr,&data);

    SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel(N, SMI_INT, SMI_ADD, 0,root,my_rank,num_ranks);
    for(int i=0;i<N;i++)
    {
        int to_comm, to_rcv=0;
        to_comm=my_rank+1;
       // printf("Rank %d sending %.0f\n",my_rank,to_comm);
        SMI_Reduce(&rchan_float,&to_comm, &to_rcv);
        if(my_rank==root)
        {
            //printf("Reduced value: %.0f\n",to_rcv);
            check &= (to_rcv==exp);
        }

    }
    *mem=check;
}



#if 0
OLD VERSION
__kernel void app(const int N, char root,char my_rank, char num_ranks, __global volatile char *mem)
{
    float exp=(num_ranks*(num_ranks+1))/2;
    char check=1;
    SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel(N, SMI_FLOAT, root,my_rank,num_ranks);
    for(int i=0;i<N;i++)
    {
        float to_comm, to_rcv=0;
        to_comm=my_rank+1;
       // printf("Rank %d sending %.0f\n",my_rank,to_comm);
        SMI_Reduce(&rchan_float,&to_comm, &to_rcv);
        if(my_rank==root)
            check &= (to_rcv==exp);

    }
    *mem=check;
}

#endif

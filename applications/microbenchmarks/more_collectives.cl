#pragma OPENCL EXTENSION cl_intel_channels : enable

//#include "smi_broadcast.h"
#include "more_collectives/smi-device-0.h"

#if 0
//two reduces :works
__kernel void app(__global char* mem, __global char* mem2, const int N, char root,char my_rank, char num_ranks/*, __global int * restrict  data*/)
{
    char check=1;
    char check2=1;
    float exp=(num_ranks*(num_ranks+1))/2;
    SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel(N, SMI_FLOAT, 0,root,my_rank,num_ranks);
    SMI_RChannel  __attribute__((register)) rchan2= SMI_Open_reduce_channel(N, SMI_INT,1, root,my_rank,num_ranks);
    for(int i=0;i<N;i++)
    {

        float to_reduce, to_rcv=0;
        to_reduce=my_rank+1;
        // printf("Rank %d sending %.0f\n",my_rank,to_comm);
        SMI_Reduce(&rchan_float,&to_reduce, &to_rcv);
        if(my_rank==root)
        {
         //   printf("Reduced value: %.0f\n",to_rcv);
            check &= (to_rcv==exp);
        }

        float to_reduce2=my_rank+2, to_rcv2;
        SMI_Reduce(&rchan2,&to_reduce2, &to_rcv2);
        if(my_rank==root)
        {
         //   printf("Reduced value: %.0f\n",to_rcv);
            check &= (to_rcv2==exp);
        }


    }

    *mem=check;
    *mem2=check2;

}

#endif
//reduce and broadcast: does not work
__kernel void app(__global char* mem, __global char* mem2, const int N, char root,char my_rank, char num_ranks/*, __global int * restrict  data*/)
{
    char check=1;
    char check2=1;
    float exp=(num_ranks*(num_ranks+1))/2;

     SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_INT,0, root,my_rank,num_ranks);
    for(int i=0;i<N;i++)
    {
        //chan.port=0;  //I need to enable this, to overwrite the port to be sure that the right circuitry is placed
        int to_comm=i;
        SMI_Bcast(&chan,&to_comm);

          check2 &= (to_comm==i);
    }


    SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel(N, SMI_FLOAT, 1,root,my_rank,num_ranks);
    for(int i=0;i<N;i++)
    {

        float to_reduce, to_rcv=0;
        to_reduce=my_rank+1;
        // printf("Rank %d sending %.0f\n",my_rank,to_comm);
        SMI_Reduce(&rchan_float,&to_reduce, &to_rcv);
        if(my_rank==root)
        {
         //   printf("Reduced value: %.0f\n",to_rcv);
            check &= (to_rcv==exp);
        }


    }

    *mem=check;
    *mem2=check2;

}
#if 0
__kernel void app2(__global char* mem, __global char* mem2, const int N, char root,char my_rank, char num_ranks/*, __global int * restrict  data*/)
{
    char check=1;
    char check2=1;
    float exp=(num_ranks*(num_ranks+1))/2;
    SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel(N, SMI_FLOAT, 1,root,my_rank,num_ranks);
    for(int i=0;i<N;i++)
    {

        float to_reduce, to_rcv=0;
        to_reduce=my_rank+1;
        // printf("Rank %d sending %.0f\n",my_rank,to_comm);
        SMI_Reduce(&rchan_float,&to_reduce, &to_rcv);
        if(my_rank==root)
        {
         //   printf("Reduced value: %.0f\n",to_rcv);
            check &= (to_rcv==exp);
        }


    }

    *mem=check;
    *mem2=check2;
}
#endif
#if 0
//three
__kernel void app(__global char* mem, __global char* mem2,__global char* mem3, const int N, char root,char my_rank, char num_ranks/*, __global int * restrict  data*/)
{
    char check=1;
    char check2=1;
    char check3=1;
    float exp=(num_ranks*(num_ranks+1))/2;
    SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel(N, SMI_FLOAT, 0,root,my_rank,num_ranks);
    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_INT,1, root,my_rank,num_ranks);
     SMI_ScatterChannel  __attribute__((register)) scattchan= SMI_Open_scatter_channel(N,N, SMI_INT, 2,root,my_rank,num_ranks);
    for(int i=0;i<N;i++)
    {

        float to_reduce, to_rcv=0;
        to_reduce=my_rank+1;
        // printf("Rank %d sending %.0f\n",my_rank,to_comm);
        SMI_Reduce(&rchan_float,&to_reduce, &to_rcv);
        if(my_rank==root)
        {
         //   printf("Reduced value: %.0f\n",to_rcv);
            check &= (to_rcv==exp);
        }

        int to_comm=i;//count_updated[i];
        SMI_Bcast(&chan,&to_comm);

          check2 &= (to_comm==i);

        int to_send,to_rcv_scatt;
        if(my_rank==root)
        {
            to_send=i;
        }

        SMI_Scatter(&scattchan,&to_send, &to_rcv_scatt);
        //check&=(to_rcv!=to_rcv_start+i);
        if(i!=to_rcv_scatt)
          check3=0;

    }

    *mem=check;
    *mem2=check2;
    *mem3=check3;

}
#endif
#if 0
//two broadcast: works

__kernel void app(__global char* mem, __global char* mem2, const int N, char root,char my_rank, char num_ranks/*, __global int * restrict  data*/)
{
    char check=1;
    char check2=1;

    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_INT,0, root,my_rank,num_ranks);
    SMI_BChannel  __attribute__((register)) chan2= SMI_Open_bcast_channel(N, SMI_FLOAT,1, root,my_rank,num_ranks);
    for(int i=0;i<N;i++)
    {

        int to_comm=i;//count_updated[i];
        float to_comm2=i;
        SMI_Bcast(&chan,&to_comm);

           check &= (to_comm==i);
        SMI_Bcast(&chan2,&to_comm2);

            check2 &= (to_comm2==i);

    }

    *mem=check;
    *mem2=check2;

}
#endif

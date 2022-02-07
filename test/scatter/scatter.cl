/**
  Scatter test. The root scatters N*NUM_RANKS elements
  accross all the ranks. Cheks is performed on each rank
*/

#include <smi.h>

#include "smi_generated_device.cl"

__kernel void test_int(const int N, char root,__global char* mem, SMI_Comm comm)
{

    SMI_ScatterChannel  __attribute__((register)) chan= SMI_Open_scatter_channel(N,N, SMI_INT, 0,root,comm);
    char check=1;
    int num_ranks=SMI_Comm_size(comm);
    int my_rank=SMI_Comm_rank(comm);
    const int loop_bound=(my_rank==root)?N*num_ranks:N;
    const int to_rcv_start=my_rank*N;
    for(int i=0;i<loop_bound;i++)
    {
        int to_send, to_rcv;
        if(my_rank==root)
            to_send=i;

        SMI_Scatter(&chan,&to_send, &to_rcv);
        if(my_rank!=root && to_rcv_start+i!=to_rcv)
          check=0;
    }
    *mem=check;
}


__kernel void test_float(const int N, char root,__global char* mem, SMI_Comm comm)
{

    SMI_ScatterChannel  __attribute__((register)) chan= SMI_Open_scatter_channel(N,N, SMI_FLOAT, 1,root,comm);
    char check=1;
    int num_ranks=SMI_Comm_size(comm);
    int my_rank=SMI_Comm_rank(comm);
    const int loop_bound=(my_rank==root)?N*num_ranks:N;
    const float to_rcv_start=my_rank*N;
    for(int i=0;i<loop_bound;i++)
    {
        float to_send, to_rcv;
        if(my_rank==root)
            to_send=i;

        SMI_Scatter(&chan,&to_send, &to_rcv);
        if(my_rank!=root && to_rcv_start+i!=to_rcv)
          check=0;
    }
    *mem=check;
}



__kernel void test_double(const int N, char root,__global char* mem, SMI_Comm comm)
{

    SMI_ScatterChannel  __attribute__((register)) chan= SMI_Open_scatter_channel(N,N, SMI_DOUBLE, 2,root,comm);
    char check=1;
    int num_ranks=SMI_Comm_size(comm);
    int my_rank=SMI_Comm_rank(comm);
    const int loop_bound=(my_rank==root)?N*num_ranks:N;
    const double to_rcv_start=my_rank*N;
    for(int i=0;i<loop_bound;i++)
    {
        double to_send, to_rcv;
        if(my_rank==root)
            to_send=i;

        SMI_Scatter(&chan,&to_send, &to_rcv);
        if(my_rank!=root && to_rcv_start+i!=to_rcv)
          check=0;
    }
    *mem=check;
}


__kernel void test_char(const int N, char root,__global char* mem, SMI_Comm comm)
{

    SMI_ScatterChannel  __attribute__((register)) chan= SMI_Open_scatter_channel(N,N, SMI_CHAR, 3,root,comm);
    //this is slightly different to take into account the limited range of chars
    char check=1;
    int num_ranks=SMI_Comm_size(comm);
    int my_rank=SMI_Comm_rank(comm);
    const int loop_bound=(my_rank==root)?N*num_ranks:N;
    const char expected=my_rank;
    int sent_to_this_rank=0;
    int rank=0;
    for(int i=0;i<loop_bound;i++)
    {
        char to_send, to_rcv;
        if(my_rank==root)
            to_send=rank;

        SMI_Scatter(&chan,&to_send, &to_rcv);
        if(my_rank!=root && expected!=to_rcv)
          check=0;
        sent_to_this_rank++;
        if(my_rank==root && sent_to_this_rank==N)
        {
            sent_to_this_rank=0;
            rank++;
        }
    }
    *mem=check;
}

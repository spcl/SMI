/*
    Gather benchmark: each rank sends its contribution (sequence of numbers) for the gather.
    The root checks the correctness of the result
*/

#include <smi.h>

__kernel void test_char(const int N, char root, __global char *mem, SMI_Comm comm)
{
    SMI_GatherChannel  __attribute__((register)) chan= SMI_Open_gather_channel(N,N, SMI_CHAR,0, root,comm);
    int my_rank=SMI_Comm_rank(comm);
    int num_ranks=SMI_Comm_size(comm);
    const int loop_bound=(my_rank==root)?N*num_ranks:N;
    char to_send=my_rank;    //starting point
    char exp=0;
    char check=1;
    int rcv=0;
    for(int i=0;i<loop_bound;i++)
    {
        char to_rcv;
        SMI_Gather(&chan,&to_send, &to_rcv);
        if(my_rank==root)
        {
            check&=(to_rcv==exp);
            rcv++;
            if(rcv==N){
                rcv=0;
                exp++;
            }
        }
    }
    *mem=check;
}

__kernel void test_short(const int N, char root, __global char *mem, SMI_Comm comm)
{
    SMI_GatherChannel  __attribute__((register)) chan= SMI_Open_gather_channel(N,N, SMI_SHORT,1, root,comm);
    int my_rank=SMI_Comm_rank(comm);
    int num_ranks=SMI_Comm_size(comm);
    const int loop_bound=(my_rank==root)?N*num_ranks:N;
    short to_send=my_rank;    //starting point
    short exp=0;
    char check=1;
    int rcv=0;
    for(int i=0;i<loop_bound;i++)
    {
        short to_rcv;
        SMI_Gather(&chan,&to_send, &to_rcv);
        if(my_rank==root)
        {
            check&=(to_rcv==exp);
            rcv++;
            if(rcv==N){
                rcv=0;
                exp++;
            }
        }
    }
    *mem=check;
}
__kernel void test_int(const int N, char root, __global char *mem, SMI_Comm comm)
{
    SMI_GatherChannel  __attribute__((register)) chan= SMI_Open_gather_channel(N,N, SMI_INT,2, root,comm);
    int my_rank=SMI_Comm_rank(comm);
    int num_ranks=SMI_Comm_size(comm);
    const int loop_bound=(my_rank==root)?N*num_ranks:N;
    int to_send=(my_rank==root)?0:my_rank*N;    //starting point
    char check=1;
    for(int i=0;i<loop_bound;i++)
    {
        int to_rcv;
        SMI_Gather(&chan,&to_send, &to_rcv);
        to_send++;
        if(my_rank==root)
            check&=(to_rcv==i);
    }
    *mem=check;
}

__kernel void test_float(const int N, char root, __global char *mem, SMI_Comm comm)
{
    SMI_GatherChannel  __attribute__((register)) chan= SMI_Open_gather_channel(N,N, SMI_FLOAT,3, root,comm);
    int my_rank=SMI_Comm_rank(comm);
    int num_ranks=SMI_Comm_size(comm);
    const int loop_bound=(my_rank==root)?N*num_ranks:N;
    float to_send=(my_rank==root)?0:my_rank*N;    //starting point
    char check=1;
    for(int i=0;i<loop_bound;i++)
    {
        float to_rcv;
        SMI_Gather(&chan,&to_send, &to_rcv);
        to_send++;
        if(my_rank==root)
            check&=(to_rcv==i);
    }
    *mem=check;
}

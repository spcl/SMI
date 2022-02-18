/**
    Reduce benchmark: each ranks sends it's contribution (its rank id).
    The root checks that the result is correct
*/

#include <smi.h>

#include "smi_generated_device.cl"

__kernel void test_float_add(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    char check=1;

    SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel(N, SMI_FLOAT, SMI_ADD, 0,root,comm);
    for(int i=0;i<N;i++)
    {
        float to_comm, to_rcv=0;
        to_comm=i; //everyone sends i
        SMI_Reduce(&rchan_float,&to_comm, &to_rcv);
        if(my_rank==root)
            check &= (to_rcv==num_ranks*i);
    }
    *mem=check;
}

__kernel void test_int_max(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    int exp=num_ranks;
    char check=1;

    SMI_RChannel  __attribute__((register)) rchan_int= SMI_Open_reduce_channel(N, SMI_INT, SMI_MAX, 2,root,comm);
    for(int i=0;i<N;i++)
    {
        int to_comm, to_rcv=0;
        to_comm=my_rank+1;
        SMI_Reduce(&rchan_int,&to_comm, &to_rcv);
        if(my_rank==root)
            check &= (to_rcv==exp);
    }
    *mem=check;
}

__kernel void test_int_add(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    int exp=(num_ranks*(num_ranks+1))/2;
    char check=1;

    SMI_RChannel  __attribute__((register)) rchan_int= SMI_Open_reduce_channel(N, SMI_INT, SMI_ADD, 1,root,comm);
    for(int i=0;i<N;i++)
    {
        int to_comm, to_rcv=0;
        to_comm=my_rank+1;
        SMI_Reduce(&rchan_int,&to_comm, &to_rcv);
        if(my_rank==root)
            check &= (to_rcv==exp);
    }
    *mem=check;
}

__kernel void test_float_min(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    char check=1;

    SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel(N, SMI_FLOAT, SMI_MIN, 3,root,comm);
    for(int i=0;i<N;i++)
    {
        float to_comm, to_rcv=0;
        to_comm=i+0.1*my_rank; //everyone sends i
        SMI_Reduce(&rchan_float,&to_comm, &to_rcv);
        if(my_rank==root)
            check &= (to_rcv==i);
    }
    *mem=check;
}

__kernel void test_int_max_ad(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    int exp=num_ranks;
    char check=1;

    SMI_RChannel  __attribute__((register)) rchan_int= SMI_Open_reduce_channel_ad(N, SMI_INT, SMI_MAX, 4,root,comm,1);
    for(int i=0;i<N;i++)
    {
        int to_comm, to_rcv=0;
        to_comm=my_rank+1;
        SMI_Reduce(&rchan_int,&to_comm, &to_rcv);
        if(my_rank==root)
            check &= (to_rcv==exp);
    }
    *mem=check;
}

__kernel void test_float_min_ad(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    char check=1;

    SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel_ad(N, SMI_FLOAT, SMI_MIN, 5,root,comm,1);
    for(int i=0;i<N;i++)
    {
        float to_comm, to_rcv=0;
        to_comm=i+0.1*my_rank; //everyone sends i
        SMI_Reduce(&rchan_float,&to_comm, &to_rcv);
        if(my_rank==root)
            check &= (to_rcv==i);
    }
    *mem=check;
}


__kernel void test_double_add(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    char check=1;

    SMI_RChannel  __attribute__((register)) rchan_float= SMI_Open_reduce_channel(N, SMI_DOUBLE, SMI_ADD, 6,root,comm);
    for(int i=0;i<N;i++)
    {
        double to_comm, to_rcv=0;
        to_comm=i; //everyone sends i
        SMI_Reduce(&rchan_float,&to_comm, &to_rcv);
        if(my_rank==root)
            check &= (to_rcv==num_ranks*i);
    }
    *mem=check;
}

__kernel void test_char_max(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    char exp=num_ranks;
    char check=1;

    SMI_RChannel  __attribute__((register)) rchan_int= SMI_Open_reduce_channel(N, SMI_CHAR, SMI_MAX, 7,root,comm);
    for(int i=0;i<N;i++)
    {
        char to_comm, to_rcv=0;
        to_comm=my_rank+1;
        SMI_Reduce(&rchan_int,&to_comm, &to_rcv);
        if(my_rank==root)
            check &= (to_rcv==exp);
    }
    *mem=check;
}
__kernel void test_short_min(const int N, char root, __global volatile char *mem, SMI_Comm comm)
{
    unsigned int my_rank=SMI_Comm_rank(comm);
    unsigned int num_ranks=SMI_Comm_size(comm);
    short exp=1;
    char check=1;

    SMI_RChannel  __attribute__((register)) rchan_int= SMI_Open_reduce_channel(N, SMI_SHORT, SMI_MIN, 8,root,comm);
    for(int i=0;i<N;i++)
    {
        short to_comm, to_rcv=0;
        to_comm=my_rank+1;
        SMI_Reduce(&rchan_int,&to_comm, &to_rcv);
        if(my_rank==root)
            check &= (to_rcv==exp);
    }
    *mem=check;
}

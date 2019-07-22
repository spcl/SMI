/**
    P2P test. Rank 0 sends a stream of data
*/
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include <smi.h>

__kernel void test_char(const int N, const char dest_rank, const SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_send_channel(N,SMI_CHAR,dest_rank,1,comm);
    for(int i=0;i<N;i++)
    {
       char send=3;
       SMI_Push(&chan,&send);
    }
}
__kernel void test_short(const int N, const char dest_rank, const SMI_Comm comm)
{

    SMI_Channel chan=SMI_Open_send_channel(N,SMI_SHORT,dest_rank,0,comm);
    for(int i=0;i<N;i++)
    {
       short send=1001;
       SMI_Push(&chan,&send);
    }
}
__kernel void test_int(const int N, const char dest_rank, const SMI_Comm comm)
{

    SMI_Channel chan=SMI_Open_send_channel(N,SMI_INT,dest_rank,2,comm);
    for(int i=0;i<N;i++)
    {
       int send=i;
       SMI_Push(&chan,&send);
    }
}

__kernel void test_float(const int N, const char dest_rank, const SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_send_channel(N,SMI_FLOAT,dest_rank,3,comm);
    const float start=1.1f;
    for(int i=0;i<N;i++)
    {
       float send=i+start;
       SMI_Push(&chan,&send);
    }
}

__kernel void test_double(const int N, const char dest_rank, const SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_send_channel(N,SMI_DOUBLE,dest_rank,4,comm);
    const double start=1.1f;
    for(int i=0;i<N;i++)
    {
       double send=i+start;
       SMI_Push(&chan,&send);
    }
}


__kernel void test_int_ad_1(const int N, const char dest_rank, const SMI_Comm comm)
{

    SMI_Channel chan=SMI_Open_send_channel_ad(N,SMI_INT,dest_rank,5,comm,1); //min asynch degree
    for(int i=0;i<N;i++)
    {
       int send=i;
       SMI_Push(&chan,&send);
    }
}

__kernel void test_int_ad_2(const int N, const char dest_rank, const SMI_Comm comm)
{

    SMI_Channel chan=SMI_Open_send_channel_ad(N,SMI_INT,dest_rank,6,comm,711); //strange asynch degree
    for(int i=0;i<N;i++)
    {
       int send=i;
       SMI_Push(&chan,&send);
    }
}

__kernel void test_char_ad_1(const int N, const char dest_rank, const SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_send_channel_ad(N,SMI_CHAR,dest_rank,7,comm,1001);
    for(int i=0;i<N;i++)
    {
       char send=3;
       SMI_Push(&chan,&send);
    }
}

__kernel void test_short_ad_1(const int N, const char dest_rank, const SMI_Comm comm)
{

    SMI_Channel chan=SMI_Open_send_channel_ad(N,SMI_SHORT,dest_rank,8,comm,0);
    for(int i=0;i<N;i++)
    {
       short send=1001;
       SMI_Push(&chan,&send);
    }
}

__kernel void test_float_ad_1(const int N, const char dest_rank, const SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_send_channel_ad(N,SMI_FLOAT,dest_rank,9,comm,13);
    const float start=1.1f;
    for(int i=0;i<N;i++)
    {
       float send=i+start;
       SMI_Push(&chan,&send);
    }
}

__kernel void test_double_ad_1(const int N, const char dest_rank, const SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_send_channel_ad(N,SMI_DOUBLE,dest_rank,10,comm,2014);
    const double start=1.1f;
    for(int i=0;i<N;i++)
    {
       double send=i+start;
       SMI_Push(&chan,&send);
    }
}

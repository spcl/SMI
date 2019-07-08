/**
	Broadcast test. A sequeuence of number is broadcasted.
	Non-root ranks check whether the received number is correct
*/

#pragma OPENCL EXTENSION cl_intel_channels : enable
#include "broadcast_routing/smi-device-0.h"



__kernel void test_int(__global char* mem, const int N, char root,SMI_Comm comm)
{
    char check=1;
    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_INT,0, root,comm);
    for(int i=0;i<N;i++)
    {

        int to_comm=i;
        SMI_Bcast(&chan,&to_comm);
        check &= (to_comm==i);
    }
    *mem=check;
}

__kernel void test_float(__global char* mem, const int N, char root,SMI_Comm comm)
{
    char check=1;
    const float offset=0.1f;
    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_FLOAT,1, root,comm);
    for(int i=0;i<N;i++)
    {

        float to_comm=i+offset;
        SMI_Bcast(&chan,&to_comm);
        check &= (to_comm==i+offset);
    }
    *mem=check;
}

__kernel void test_double(__global char* mem, const int N, char root,SMI_Comm comm)
{
    char check=1;
    const double offset=0.1f;
    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_DOUBLE,2, root,comm);
    for(int i=0;i<N;i++)
    {

        double to_comm=i+offset;
        SMI_Bcast(&chan,&to_comm);
        check &= (to_comm==i+offset);
    }
    *mem=check;
}

__kernel void test_char(__global char* mem, const int N, char root,SMI_Comm comm)
{
    char check=1;
    char expected=root;
    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_CHAR,3, root,comm);
    for(int i=0;i<N;i++)
    {

        char to_comm=expected;
        SMI_Bcast(&chan,&to_comm);
        check &= (to_comm==expected);
    }
    *mem=check;
}

__kernel void test_short(__global char* mem, const int N, char root,SMI_Comm comm)
{
    char check=1;
    short expected=root;
    SMI_BChannel  __attribute__((register)) chan= SMI_Open_bcast_channel(N, SMI_SHORT,4, root,comm);
    for(int i=0;i<N;i++)
    {

        short to_comm=expected;
        SMI_Bcast(&chan,&to_comm);
        check &= (to_comm==expected);
    }
    *mem=check;
}



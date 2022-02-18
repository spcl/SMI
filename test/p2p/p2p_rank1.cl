/**
    P2P test:
    RANK 0 is the source of the data, RANK 1 check that correct data is received
*/
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include <smi.h>
#include "smi_generated_device.cl"

__kernel void test_short(__global char *mem, const int N, SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_SHORT,0,0,comm);
    char check=1;
    const short expected=1001;
    for(int i=0;i<N;i++)
    {
        short rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(expected));
    }
    *mem=check;

}

__kernel void test_char(__global char *mem, const int N, SMI_Comm comm)
{

    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_CHAR,0,1,comm);
    char check=1;
    //since it is char, we send always the same number
    const char expected=3;
    for(int i=0;i<N;i++)
    {
        char rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(expected));
    }
    *mem=check;

}

__kernel void test_int(__global char *mem, const int N, SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_INT,0,2,comm);
    char check=1;
    for(int i=0;i<N;i++)
    {
        int rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(i));
    }
    *mem=check;

}

__kernel void test_float(__global char *mem, const int N, SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_FLOAT,0,3,comm);
    char check=1;
    const float start=1.1f;
    for(int i=0;i<N;i++)
    {
        float rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(i+start));
    }

    *mem=check;

}

__kernel void test_double(__global char *mem, const int N, SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_DOUBLE,0,4,comm);
    char check=1;
    const double start=1.1f;
    for(int i=0;i<N;i++)
    {
        double rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(i+start));
    }
    *mem=check;

}

__kernel void test_int_ad_1(__global char *mem, const int N, SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_receive_channel_ad(N,SMI_INT,0,5,comm,1);
    char check=1;
    for(int i=0;i<N;i++)
    {
        int rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(i));
    }
    *mem=check;

}

__kernel void test_int_ad_2(__global char *mem, const int N, SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_receive_channel_ad(N,SMI_INT,0,6,comm,711);
    char check=1;
    for(int i=0;i<N;i++)
    {
        int rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(i));
    }
    *mem=check;

}


__kernel void test_char_ad_1(__global char *mem, const int N, SMI_Comm comm)
{

    SMI_Channel chan=SMI_Open_receive_channel_ad(N,SMI_CHAR,0,7,comm,1001);
    char check=1;
    //since it is char, we send always the same number
    const char expected=3;
    for(int i=0;i<N;i++)
    {
        char rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(expected));
    }
    *mem=check;

}
__kernel void test_short_ad_1(__global char *mem, const int N, SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_receive_channel_ad(N,SMI_SHORT,0,8,comm,0);
    char check=1;
    const short expected=1001;
    for(int i=0;i<N;i++)
    {
        short rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(expected));
    }
    *mem=check;

}

__kernel void test_float_ad_1(__global char *mem, const int N, SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_receive_channel_ad(N,SMI_FLOAT,0,9,comm,13);
    char check=1;
    const float start=1.1f;
    for(int i=0;i<N;i++)
    {
        float rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(i+start));
    }

    *mem=check;

}

__kernel void test_double_ad_1(__global char *mem, const int N, SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_receive_channel_ad(N,SMI_DOUBLE,0,10,comm,2014);
    char check=1;
    const double start=1.1f;
    for(int i=0;i<N;i++)
    {
        double rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(i+start));
    }
    *mem=check;

}

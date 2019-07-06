/**
    P2P test:
    RANK 0 is the source of the data, RANK 1 check that correct data is received
*/
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#include "p2p_routing/smi-device-1.h"

__kernel void test_int(__global char *mem, const int N, SMI_Comm comm)
{
    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_INT,0,0,comm);
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
    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_FLOAT,0,0,comm);
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

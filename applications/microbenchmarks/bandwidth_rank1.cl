/**
    Bandwidth microbenchmark

    The benchmark is constituted by two ranks:
    - rank 0 sends data
    - rank 1 receives it

    The data sent is a message of N double. We can
    enable up to three senders/receivers.
    By sending double, in each network packet there can be
    fit only three of them. So 2/3 sender should be sufficient
    to saturate the bandwidth of the single QSFP

    In this first version, there are all the CK_S/CK_Rs
    Applications are attached to:
    - a single CK_S/CK_R if the programs are compiled
        with the macro SINGLE_CK. This can be used as stress
        test of the single CK_S/CK_R

    We use only one QSFP (e.g. QSFP0 in the case we are
    using FPGA-0014)
*/

#include "smi_bandwidth_rank1.h"
#define FLOAT
#if defined (FLOAT)
    #define TYPE_T float
    #define SMI_TYPE_T SMI_FLOAT
#else
    #define TYPE_T double
    #define SMI_TYPE_T SMI_DOUBLE
#endif



__kernel void app_0(__global volatile char *mem, const int N)
{
    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_TYPE_T,0,0);
    const TYPE_T start=0.1f;
    char check=1;
    for(int i=0;i<N;i++)
    {

        TYPE_T rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(start+i));
    }
    *mem=check;
}

__kernel void app_1(__global volatile char *mem, const int N)
{
    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_TYPE_T,0,1);
    const TYPE_T start=1.1f;
    char check=1;
    for(int i=0;i<N;i++)
    {

        TYPE_T rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(start+i));
    }
    *mem=check;
}

__kernel void app_2(__global volatile char *mem, const int N)
{
    SMI_Channel chan=SMI_Open_receive_channel(N,SMI_TYPE_T,0,2);
    const TYPE_T start=2.1f;
    char check=1;
    for(int i=0;i<N;i++)
    {

        TYPE_T rcvd;
        SMI_Pop(&chan,&rcvd);
        check &= (rcvd==(start+i));
    }
    *mem=check;

}

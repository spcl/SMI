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

#include "smi_bandwidth_rank0.h"

__kernel void app_0(const int N)
{
    SMI_Channel chan=SMI_Open_send_channel(N,SMI_DOUBLE,1,0);
    const double start=0.1f;

    for(int i=0;i<N;i++)
    {
        double send=start+i;
        SMI_Push(&chan,&send);
    }

}

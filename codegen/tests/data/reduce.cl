#pragma OPENCL EXTENSION cl_intel_channels : enable

#include <smi.h>

__kernel void app_0(const int N, const char dst)
{
    SMI_Comm comm;
    for (int i = 0; i < N; i++)
    {
        SMI_RChannel chan_reduce = SMI_Open_reduce_channel(1, SMI_INT, SMI_ADD, 0, 1, comm);
        SMI_Reduce(&chan_reduce, &i, &i);

        SMI_RChannel chan_reduce1 = SMI_Open_reduce_channel(1, SMI_INT, SMI_MIN, 1, 1, comm);
        SMI_Reduce(&chan_reduce1, &i, &i);

        SMI_RChannel chan_reduce2 = SMI_Open_reduce_channel(1, SMI_INT, SMI_MAX, 2, 1, comm);
        SMI_Reduce(&chan_reduce2, &i, &i);
    }
}

#pragma OPENCL EXTENSION cl_intel_channels : enable

#include <smi.h>

__kernel void app_0(const int N, const char dst)
{
    SMI_Comm comm;

#pragma unroll
    for (int i = 0; i < N; i++)
    {
        float16 var1;
        uint var2;
        // push
        SMI_Channel chan_send = SMI_Open_send_channel(1, SMI_CHAR, dst, 0, comm);
        SMI_Push(&chan_send, &var1);

        // pop
        SMI_Channel chan_recv = SMI_Open_receive_channel(1, SMI_DOUBLE, dst, 1, comm);
        SMI_Pop(&chan_recv, &var2);

        // broadcast
        SMI_BChannel chan_bcast = SMI_Open_bcast_channel(1, SMI_INT, 2, 1, comm);
        SMI_Bcast(&chan_bcast, &i, &i);

        // reduce
        SMI_RChannel chan_reduce = SMI_Open_reduce_channel(1, SMI_FLOAT, SMI_ADD, 3, 1, comm);
        SMI_Reduce(&chan_reduce, &i, &i);

        // scatter
        SMI_ScatterChannel chan_scatter = SMI_Open_scatter_channel(1, 1, SMI_SHORT, 4, 1, comm);
        SMI_Scatter(&chan_scatter, &i, &i);

        // gather
        SMI_GatherChannel chan_gather = SMI_Open_gather_channel(1, 1, SMI_INT, 5, 1, comm);
        SMI_Gather(&chan_gather, &i, &i);
    }
}

#pragma OPENCL EXTENSION cl_intel_channels : enable

#include <smi.h>

__kernel void app_0(const int N, const char dst)
{
    SMI_Comm comm;
    for (int i = 0; i < N; i++)
    {
        SMI_Channel chan_send = SMI_Open_send_channel(1, SMI_INT, dst, 0, comm);
        SMI_Push(&chan_send, &i);
        SMI_Channel chan_recv = SMI_Open_receive_channel(1, SMI_INT, dst, 3, comm);
        SMI_Pop(&chan_recv, &i);
    }
}

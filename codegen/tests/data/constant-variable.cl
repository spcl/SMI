#pragma OPENCL EXTENSION cl_intel_channels : enable

#include <smi.h>

__kernel void app_0(const int N, const char dst)
{
    SMI_Comm comm;
    for (int i = 0; i < N; i++)
    {
        const int port = 5;
        SMI_Channel chan_send1 = SMI_Open_send_channel(1, SMI_INT, dst, port, comm);
        SMI_Push(&chan_send1, &i);
    }
}

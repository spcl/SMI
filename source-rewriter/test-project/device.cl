#pragma OPENCL EXTENSION cl_intel_channels : enable

#define BUFFER_SIZE 4096

#include <smi/push.h>
#include <smi/test-include.h>
#include "device.h"

__kernel void app_0(const int N, const char dst)
{
    SMI_Comm comm;
    for (int i = 0; i < N + HOST_PORT; i++)
    {
        SMI_Channel chan_send1 = SMI_Open_send_channel(1, SMI_INT, dst, 0, comm);
        SMI_Push(&chan_send1, &i);
        TEST_DATA;
    }
}

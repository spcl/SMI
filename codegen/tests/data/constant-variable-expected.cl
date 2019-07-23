#pragma OPENCL EXTENSION cl_intel_channels : enable

#include <smi.h>

void SMI_Push_5_int(SMI_Channel* chan, void* data);
SMI_Channel SMI_Open_send_channel_5_int(int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm);
__kernel void app_0(const int N, const char dst)
{
    SMI_Comm comm;
    for (int i = 0; i < N; i++)
    {
        const int port = 5;
        SMI_Channel chan_send1 = SMI_Open_send_channel_5_int(1, SMI_INT, dst, port, comm);
        SMI_Push_5_int(&chan_send1, &i);
    }
}

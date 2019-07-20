#pragma OPENCL EXTENSION cl_intel_channels : enable

#include <smi.h>

void SMI_Push_0_int(SMI_Channel* chan, void* data);
SMI_Channel SMI_Open_send_channel_0_int(int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm);
__kernel void app_0(const int N, const char dst)
{
    SMI_Comm comm;
    for (int i = 0; i < N; i++)
    {
        SMI_Channel chan_send1 = SMI_Open_send_channel_0_int(1, SMI_INT, dst, 0, comm);
        SMI_Push_0_int(&chan_send1, &i);
    }
}

void SMI_Push_1_int(SMI_Channel* chan, void* data);
SMI_Channel SMI_Open_send_channel_1_int(int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm);
kernel void app_1(const int N, const char dst)
{
    SMI_Comm comm;
    for (int i = 0; i < N; i++)
    {
        SMI_Channel chan_send1 = SMI_Open_send_channel_1_int(1, SMI_INT, dst, 1, comm);
        SMI_Push_1_int(&chan_send1, &i);
    }
}

kernelx void app_2(const int N, const char dst)
{
    SMI_Comm comm;
    for (int i = 0; i < N; i++)
    {
        SMI_Channel chan_send1 = SMI_Open_send_channel(1, SMI_INT, dst, 2, comm);
        SMI_Push(&chan_send1, &i);
    }
}

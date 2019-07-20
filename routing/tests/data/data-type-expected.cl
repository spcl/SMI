#pragma OPENCL EXTENSION cl_intel_channels : enable

#include <smi.h>

void SMI_Pop_4_int(SMI_Channel* chan, void* data);
SMI_Channel SMI_Open_receive_channel_4_int(int count, SMI_Datatype data_type, int source, int port, SMI_Comm comm);
void SMI_Pop_3_float(SMI_Channel* chan, void* data);
SMI_Channel SMI_Open_receive_channel_3_float(int count, SMI_Datatype data_type, int source, int port, SMI_Comm comm);
void SMI_Pop_1_double(SMI_Channel* chan, void* data);
SMI_Channel SMI_Open_receive_channel_1_double(int count, SMI_Datatype data_type, int source, int port, SMI_Comm comm);
void SMI_Push_0_char(SMI_Channel* chan, void* data);
SMI_Channel SMI_Open_send_channel_0_char(int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm);
__kernel void app_0(const int N, const char dst)
{
    SMI_Comm comm;
    for (int i = 0; i < N; i++)
    {
        SMI_Channel chan_send = SMI_Open_send_channel_0_char(1, SMI_CHAR, dst, 0, comm);
        SMI_Push_0_char(&chan_send, &i);
        SMI_Channel chan_recv = SMI_Open_receive_channel_1_double(1, SMI_DOUBLE, dst, 1, comm);
        SMI_Pop_1_double(&chan_recv, &i);
        SMI_Channel chan_recv1 = SMI_Open_receive_channel_3_float(1, SMI_FLOAT, dst, 3, comm);
        SMI_Pop_3_float(&chan_recv1, &i);
        SMI_Channel chan_recv2 = SMI_Open_receive_channel_4_int(1, SMI_INT, dst, 4, comm);
        SMI_Pop_4_int(&chan_recv2, &i);
    }
}

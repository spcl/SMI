#pragma OPENCL EXTENSION cl_intel_channels : enable

#include <smi.h>

void SMI_Reduce_2_int(SMI_RChannel* chan,  void* data_snd, void* data_rcv);
SMI_RChannel SMI_Open_reduce_channel_2_int(int count, SMI_Datatype data_type, SMI_Op op, int port, int root, SMI_Comm comm);
void SMI_Reduce_1_int(SMI_RChannel* chan,  void* data_snd, void* data_rcv);
SMI_RChannel SMI_Open_reduce_channel_1_int(int count, SMI_Datatype data_type, SMI_Op op, int port, int root, SMI_Comm comm);
void SMI_Reduce_0_int(SMI_RChannel* chan,  void* data_snd, void* data_rcv);
SMI_RChannel SMI_Open_reduce_channel_0_int(int count, SMI_Datatype data_type, SMI_Op op, int port, int root, SMI_Comm comm);
__kernel void app_0(const int N, const char dst)
{
    SMI_Comm comm;
    for (int i = 0; i < N; i++)
    {
        SMI_RChannel chan_reduce = SMI_Open_reduce_channel_0_int(1, SMI_INT, SMI_ADD, 0, 1, comm);
        SMI_Reduce_0_int(&chan_reduce, &i, &i);

        SMI_RChannel chan_reduce1 = SMI_Open_reduce_channel_1_int(1, SMI_INT, SMI_MIN, 1, 1, comm);
        SMI_Reduce_1_int(&chan_reduce1, &i, &i);

        SMI_RChannel chan_reduce2 = SMI_Open_reduce_channel_2_int(1, SMI_INT, SMI_MAX, 2, 1, comm);
        SMI_Reduce_2_int(&chan_reduce2, &i, &i);
    }
}

#pragma OPENCL EXTENSION cl_intel_channels : enable

#include <smi.h>

void SMI_Gather_5_int(SMI_GatherChannel* chan, void* send_data, void* rcv_data);
SMI_GatherChannel SMI_Open_gather_channel_5_int(int send_count, int recv_count, SMI_Datatype data_type, int port, int root, SMI_Comm comm);
void SMI_Scatter_4_short(SMI_ScatterChannel* chan, void* data_snd, void* data_rcv);
SMI_ScatterChannel SMI_Open_scatter_channel_4_short(int send_count, int recv_count, SMI_Datatype data_type, int port, int root, SMI_Comm comm);
void SMI_Reduce_3_float(SMI_RChannel* chan,  void* data_snd, void* data_rcv);
SMI_RChannel SMI_Open_reduce_channel_3_float(int count, SMI_Datatype data_type, SMI_Op op, int port, int root, SMI_Comm comm);
SMI_BChannel SMI_Open_bcast_channel_2_int(int count, SMI_Datatype data_type, int port, int root, SMI_Comm comm);
void SMI_Pop_1_double(SMI_Channel* chan, void* data);
SMI_Channel SMI_Open_receive_channel_1_double(int count, SMI_Datatype data_type, int source, int port, SMI_Comm comm);
void SMI_Push_0_char(SMI_Channel* chan, void* data);
SMI_Channel SMI_Open_send_channel_0_char(int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm);
__kernel void app_0(const int N, const char dst)
{
    SMI_Comm comm;

#pragma unroll
    for (int i = 0; i < N; i++)
    {
        float16 var1;
        uint var2;
        // push
        SMI_Channel chan_send = SMI_Open_send_channel_0_char(1, SMI_CHAR, dst, 0, comm);
        SMI_Push_0_char(&chan_send, &var1);

        // pop
        SMI_Channel chan_recv = SMI_Open_receive_channel_1_double(1, SMI_DOUBLE, dst, 1, comm);
        SMI_Pop_1_double(&chan_recv, &var2);

        // broadcast
        SMI_BChannel chan_bcast = SMI_Open_bcast_channel_2_int(1, SMI_INT, 2, 1, comm);
        SMI_Bcast(&chan_bcast, &i, &i);

        // reduce
        SMI_RChannel chan_reduce = SMI_Open_reduce_channel_3_float(1, SMI_FLOAT, SMI_ADD, 3, 1, comm);
        SMI_Reduce_3_float(&chan_reduce, &i, &i);

        // scatter
        SMI_ScatterChannel chan_scatter = SMI_Open_scatter_channel_4_short(1, 1, SMI_SHORT, 4, 1, comm);
        SMI_Scatter_4_short(&chan_scatter, &i, &i);

        // gather
        SMI_GatherChannel chan_gather = SMI_Open_gather_channel_5_int(1, 1, SMI_INT, 5, 1, comm);
        SMI_Gather_5_int(&chan_gather, &i, &i);
    }
}

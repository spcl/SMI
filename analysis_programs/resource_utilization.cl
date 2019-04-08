/**
    Used to measure the resource utilization with different
    number of used QSFP
*/
#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "smi_resource_utilization.h"



__kernel void app(const int N)
{
    //SMI_Channel chan_send=SMI_Open_send_channel(N,SMI_INT,1,0);
    //SMI_Channel chan_receive=SMI_Open_receive_channel(N,SMI_INT,1,1);
    for(int i=0;i<N;i++)
    {
        int to_send[4];
        int to_receive[4];
        to_send[0]=i;to_send[1]=i; to_send[2]=i; to_send[3]=i;

        SMI_Channel chan_send_0=SMI_Open_send_channel(1,SMI_INT,1,0);
        SMI_Push(&chan_send_0,&to_send[0]);
        #if QSFP_COUNT > 1
        SMI_Channel chan_send_1=SMI_Open_send_channel(1,SMI_INT,1,1);
        SMI_Push(&chan_send_1,&to_send[1]);
        #if QSFP_COUNT > 2
        SMI_Channel chan_send_2=SMI_Open_send_channel(1,SMI_INT,1,2);
        SMI_Push(&chan_send_2,&to_send[2]);
        #if QSFP_COUNT > 3
        SMI_Channel chan_send_3=SMI_Open_send_channel(1,SMI_INT,1,3);
        SMI_Push(&chan_send_3,&to_send[3]);
        #endif
        #endif
        #endif

        SMI_Channel chan_receive_0=SMI_Open_receive_channel(1,SMI_INT,1,0);
        SMI_Pop(&chan_receive_0,&to_receive[0]);
        #if QSFP_COUNT > 1
        SMI_Channel chan_receive_1=SMI_Open_receive_channel(1,SMI_INT,1,1);
        SMI_Pop(&chan_receive_1,&to_receive[1]);
        #if QSFP_COUNT > 2
        SMI_Channel chan_receive_2=SMI_Open_receive_channel(1,SMI_INT,1,2);
        SMI_Pop(&chan_receive_2,&to_receive[2]);
        #if QSFP_COUNT > 3
        SMI_Channel chan_receive_3=SMI_Open_receive_channel(1,SMI_INT,1,3);
        SMI_Pop(&chan_receive_3,&to_receive[3]);
        #endif
        #endif
        #endif

    }
}

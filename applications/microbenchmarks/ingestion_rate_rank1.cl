#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "smi_ingestion_rate_rank1.h"



__kernel void app_1(const int N)
{
    int rcv;
    for(int i=0;i<N;i++)
    {
        SMI_Channel chan_receive=SMI_Open_receive_channel(1,SMI_INT,0,0);
        SMI_Pop(&chan_receive,&rcv);
        printf("Received %d\n",i);
        if(rcv!=i)
            printf("App_1, what a shame I was expecting %d and instead I received %d\n",i,rcv);
    }
}


__kernel void app_2(const int N)
{
    int rcv;
    for(int i=0;i<N;i++)
    {
        SMI_Channel chan_receive=SMI_Open_receive_channel(1,SMI_INT,0,1);
        SMI_Pop(&chan_receive,&rcv);
        //if(rcv!=i)
        //    printf("App_2, what a shame I was expecting %d and instead I received %d\n",i,rcv);
    }
}

/**
Instead of the loop we have just a bunch of send
followed by a bunch of receive
*/

#include "smi_latency.h"



__kernel void app(const int N,const int M)
{
    //SMI_Channel chan_send=SMI_Open_send_channel(N,SMI_INT,1,0);
    //SMI_Channel chan_receive=SMI_Open_receive_channel(N,SMI_INT,1,1);
    int to_send=0;
    for(int i=0;i<N;i++)
    {
        //SMI_Push_flush(&chan_send,&to_send,true);
        SMI_Channel chan_send=SMI_Open_send_channel(1,SMI_INT,1,0);
        SMI_Push(&chan_send,&to_send);
        to_send++;
    }
    for(int i=0;i<M;i++)
    {
        SMI_Channel chan_receive=SMI_Open_receive_channel(1,SMI_INT,1,1);
        SMI_Pop(&chan_receive,&to_send);
    }
}

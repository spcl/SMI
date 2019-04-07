/**
    Latency microbenchmark

    The benchmark is constituted by two ranks:
    - rank 0 sends data
    - rank 1 receives it

    Each one waits for the counterpart.
    This round-trip is done N times so the latency is the total time
    spent/2N

    For the moment being we impose a dependency in the loop (we send what we receive)
    I don't know if it is necessary or not

    We use only one QSFP (e.g. QSFP0 in the case we are
    using FPGA-0014)
*/

#include "smi_latency.h"



__kernel void app(const int N)
{
    //SMI_Channel chan_send=SMI_Open_send_channel(N,SMI_INT,0,1);
    //SMI_Channel chan_receive=SMI_Open_receive_channel(N,SMI_INT,0,0);
    for(int i=0;i<N;i++)
    {
        int to_send=i;
        int to_receive;
        SMI_Channel chan_send=SMI_Open_send_channel(1,SMI_INT,0,1);
        SMI_Channel chan_receive=SMI_Open_receive_channel(1,SMI_INT,0,0);
        SMI_Pop(&chan_receive,&to_receive);
        //SMI_Push_flush(&chan_send,&to_send,true);
        SMI_Push(&chan_send,&to_send);
        to_send++;
    }
}

/**
    Receiver for the send_receive_no_memory benchmark
    The RECEIVER will receive the stream and checks wheter it is ordered or not.

    The message is constituted by an int8
*/
#pragma OPENCL EXTENSION cl_intel_channels : enable

typedef struct{
    int8 data;
}message_t;

///#define EMULATION
#if defined (EMULATION)
channel message_t chan_in __attribute__((depth(16)))
                           __attribute__((io("emulationIOchannel")));

#else
channel message_t chan_in __attribute__((depth(16)))
                           __attribute__((io("kernel_input_ch0")));

#endif


__kernel void receiver(__global volatile char *mem, const int N)
{
    const int loop_it=1 + (int)((N-1) /8);
    char check=1;
    for(int i=0;i<loop_it;i++)
    {
        message_t mess= read_channel_intel(chan_in);
        for(int j=0;j<8;j++)
        {
            check &= (mess.data[j]==i*8+j); //check that data is arrived in order
           // printf("[%d] Received: %d\n",i*8+j,mess.data[j]);
        }
    }
    *mem=check;
}

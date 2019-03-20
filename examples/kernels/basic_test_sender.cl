/**
    Basic test
    The SENDER will generates a stream of integer from 0 to N-1
    The RECEIVER will receive the stream and checks wheter it is ordered or not.

    The message is constituted by an int8
*/
#pragma OPENCL EXTENSION cl_intel_channels : enable

typedef struct{
    int8 data;
}message_t;

//#define EMULATION
#if defined (EMULATION)

channel message_t chan_out __attribute__((depth(16)))
                           __attribute__((io("emulationIOchannel")));
#else

channel message_t chan_out __attribute__((depth(16)))
                           __attribute__((io("kernel_output_ch0")));
#endif

__kernel void sender(const int N)
{
    const int loop_it=1 + (int)((N-1) /8);

    for(int i=0;i<loop_it;i++)
    {
        message_t mess;
        for(int j=0;j<8;j++)
        {
            mess.data[j]=i*8+j;
        }
        write_channel_intel(chan_out,mess);
    }
}

/**
    Communication Kernel - Receiver
    Receives data from I/O and forward them
    to unpackers

    NOTE: This is a skeleton, not a full implementation.
        It must be specialized with the right connections and
        number of unpackers

*/

#include "../datatypes/network_message.h"
#include "../datatypes/application_message.h"

#pragma OPENCL EXTENSION cl_intel_channels : enable

channel network_message_t io_out __attribute__((depth(16)));
channel network_message_t chan_from_ck_r[2] __attribute__((depth(16)));


__kernel void CK_receiver()
{

    while(1)
    {
        network_message_t m=read_channel_intel(io_out);
        //forward it to the right unpacker
        //TODO change this accordingly
        switch(m.header.dst)
        {
            case 0:
                write_channel_intel(chan_from_ck_r[0],m);
                break;
            case 1:
                write_channel_intel(chan_from_ck_r[1],m);
                break;
        }

    }

}



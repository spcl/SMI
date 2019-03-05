/**
    Communication Kernel - Sender
    Receives data from packers and forward them
    to IO channel

    NOTE: This is a skeleton, not a full implementation.
        It must be specialized with the right connections and
        number of packers

*/

#include "../datatypes/network_message.h"
#include "../datatypes/application_message.h"

#pragma OPENCL EXTENSION cl_intel_channels : enable

channel network_message_t io_out __attribute__((depth(16)));
channel network_message_t chan_to_ck_s[2] __attribute__((depth(16)));


__kernel void CK_sender()
{
    const uint num_sender=2;
    uint sender_id=0;
    bool valid=false;
    network_message_t mess;
    while(1)
    {
        //TODO: change this according to the number of channels that you have
        switch(sender_id)
        {
            case 0:
            mess=read_channel_nb_intel(chan_to_ck_s[0],&valid);
            break;
            case 1:
            mess=read_channel_nb_intel(chan_to_ck_s[1],&valid);
            break;
        }
        sender_id=(sender_id+1)%num_sender;
        if(valid)
            write_channel_intel(io_out,mess);

    }

}


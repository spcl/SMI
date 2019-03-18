/**
    Communication Kernel - Receiver
    Receives data from I/O and forward them
    to unpackers

    NOTE: This is a skeleton, not a full implementation.
        It must be code generated

*/

#include "../datatypes/network_message.h"

#pragma OPENCL EXTENSION cl_intel_channels : enable

/*
 * This depends on:
 * - i/o input channels definitions: they are stored into an array (suppose io_in)
 * - the external routing table: computed and passed by the host. This will be probably
 *      loaded from DRAM at the kernel startup
 * - the internal routing table
 * - channel_from_ck_r: an array of channels to application endpoint.
 *      How many channel are assigned to a CK_R and which are must be decided
 *      at compile time and proper code must be generated to handle them
 */

/*
 * TODO:
 * - termination
 */

__kernel void CK_receiver()
{

    while(1)
    {
        SMI_NetworkMessage m=read_channel_intel(io_in);
        //forward it to the right application endpoint
        const char tag=GET_HEADER_TAG(m.header);
        const char chan_id=internal_receiver_rt[tag];
        //TODO generate this accordingly
        //NOTE: the explicit switch allows the compiler to generate the hardware
        switch(chan_id)
        {
            case 0:
                write_channel_intel(channel_from_ck_r[0],m);
            break;
            case 1:
                write_channel_intel(channel_from_ck_r[1],m);
            break;
        }

    }

}



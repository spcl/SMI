/**
    Communication Kernel - Sender
    Receives data from packers and forward them
    to IO channel

    NOTE: This is a skeleton, not a full implementation.
        It must be specialized with the right connections and
        number of assigned channels

*/

#include "../datatypes/network_message.h"

#pragma OPENCL EXTENSION cl_intel_channels : enable

/* This depends on:
 * - i/o output channels definitions: they are stored into an array (suppose io_out)
 * - the routing table: computed and passed by the host. This will be probably
 *      loaded from DRAM at the kernel startup
 * - channel_to_ck_s: an array of channel in which the push writes.
 *      How many channel are assigned to a CK_S and which are must be decided
 *      at compile time and proper code must be generated to handle them
 */

/*
 * TODO:
 * - termination
 */


__kernel void CK_sender()
{
    const char num_sender=2;
    char sender_id=0;
    bool valid=false;
    SMI_NetworkMessage mess;
    while(1)
    {
        switch(sender_id)
        {
            case 0:
                mess=read_channel_nb_intel(channel_to_ck_s[0],&valid);
            break;
            case 1:
                mess=read_channel_nb_intel(channel_to_ck_s[1],&valid);
            break;
        }

        if(valid)
        {
            write_channel_intel(io_out,mess);
            valid=false;
        }
        sender_id++;
        if(sender_id==num_sender)
                sender_id=0;

    }
}




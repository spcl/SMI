/**
    Push to channel
*/

#ifndef PUSH_H
#define PUSH_H
#include "../datatypes/channel_descriptor.h"


/*
 * ATTENTION: this depends on:
 *  - the internal routing table (internal_send_rt). It maps tag to channel_id.
 *      It must be available as a global constant char array
 *  - the array of channel channel_to_ck_s
 *
 *
 */

// TODO: probably we will need some sort of Flush, to not wait for a full packet
// but this should be implemented with a particular Push (otherwise it will create multiple channel write points)


//TODO: understand where we should put this. Probably a code-generated header with the topology
extern __constant char internal_sender_rt[2];
extern channel SMI_NetworkMessage channel_to_ck_s[2];

/**
 * @brief SMI_Push push a data elements in the data channel. Data transferring can be delayed
 * @param chan
 * @param data
 * @param immediate: if true the data is immediately sent, without waiting for the completion of the network packet
 */
void SMI_PushI(SMI_Channel *chan, void* data, bool immediate)
{
    char *conv=(char*)data;
    const char chan_idx=internal_sender_rt[chan->tag];
    #pragma unroll
    for(int jj=0;jj<chan->size_of_type;jj++) //copy the data
        chan->net.data[chan->packet_element_id*chan->size_of_type+jj]=conv[jj];
    chan->processed_elements++;
    chan->packet_element_id++;
    if(chan->packet_element_id==chan->elements_per_packet || immediate || chan->processed_elements==chan->message_size) //send it if packet is filled or we reached the message size
    {
        SET_HEADER_NUM_ELEMS(chan->net.header,chan->packet_element_id);
        chan->packet_element_id=0;
        write_channel_intel(channel_to_ck_s[chan_idx],chan->net);
    }
}


void SMI_Push(SMI_Channel *chan, void* data)
{
    SMI_PushI(chan,data,false);
}



#endif //ifndef PUSH_H

/**
    Push to channel
*/

#ifndef PUSH_H
#define PUSH_H
#include "./channel_descriptor.h"


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

/**
 * @brief SMI_Push push a data elements in the data channel. Data transferring can be delayed
 * @param chan
 * @param data
 * @param immediate: if true the data is immediately sent, without waiting for the completion of the network packet
 */
void SMI_Push_flush(SMI_Channel *chan, void* data, bool immediate)
{
    if(!chan->rendezvous)
    {
        //wait until the message arrives
        const char chan_idx=internal_receiver_rt[chan->tag];
        SMI_Network_message mess=read_channel_intel(channels_from_ck_r[chan_idx]);
       // printf("Sender, recevd rendezvous from tag %d\n",chan->tag);
        chan->rendezvous =true;

    }
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
        write_channel_intel(channels_to_ck_s[chan_idx],chan->net);
    }
    
}


void SMI_Push(SMI_Channel *chan, void* data)
{
    SMI_Push_flush(chan,data,false);
}



#endif //ifndef PUSH_H

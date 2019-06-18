/**
    Pop from channel
*/

#ifndef POP_H
#define POP_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#include "./channel_descriptor.h"


/*
 * ATTENTION: this depends on:
 *  - the internal routing table (internal_receiver_rt). It maps tag to channel_id.
 *      It must be available as a global constant char array
 *  - the array of channel channel_from_ck_r
 *
 *
 */

/**
 * @brief SMI_Pop: this stalls until data arrives
 * @param chan
 * @param data
 */
void SMI_Pop(SMI_Channel *chan, void *data)
{

    //in this case we have to copy the data into the target variable
    if(chan->packet_element_id==0)
    {
        const char chan_idx_data=internal_from_ckr_data_rt[chan->port];
        //const char chan_idx=internal_receiver_rt[chan->tag];
        chan->net=read_channel_intel(channels_from_ck_r[chan_idx_data]);
    }
    char * ptr=chan->net.data+(chan->packet_element_id)*chan->size_of_type;
    chan->packet_element_id++;                       //first increment and then use it: otherwise compiler detects Fmax problems
    if(chan->packet_element_id==GET_HEADER_NUM_ELEMS(chan->net.header))
        chan->packet_element_id=0;
    //if we reached the number of elements in this packet get the next one from CK_R
    chan->processed_elements++;                      //TODO: probably useless
    //create data element
    if(chan->data_type==SMI_INT)
        *(int *)data= *(int*)(ptr);
    if(chan->data_type==SMI_FLOAT)
        *(float *)data= *(float*)(ptr);
    if(chan->data_type==SMI_DOUBLE)
        *(double *)data= *(double*)(ptr);
    chan->tokens--;
    //This is used to prevent this funny compiler to re-oder the two *_channel_intel operations

    if(chan->tokens==0) //va nell'if
    {
        //this is also useful for enforcing the ordering
        //At this point, the sender has still max_tokens*7/8 tokens: we have to consider this while we send
        //the new tokens to it
        chan->tokens=MIN(chan->max_tokens/8, MAX(chan->message_size-chan->processed_elements-chan->max_tokens*7/8,0)); //b/2

        const char chand_idx_control=internal_to_cks_control_rt[chan->port];
        SMI_Network_message mess;
        *(uint*)mess.data=chan->tokens;
        SET_HEADER_DST(mess.header,chan->sender_rank);
        SET_HEADER_TAG(mess.header,internal_sender_port_receiving[chan->port]);  //TODO this must be choosen properly, we have to map it properly
        SET_HEADER_OP(mess.header,SMI_REQUEST);
        write_channel_intel(channels_to_ck_s[chand_idx_control],mess);
        //printf("Receiver, sent tokens: %d to tag %d\n",chan->tokens,chan->tag);

    }

    
}

#endif //ifndef POP_

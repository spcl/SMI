/**
    Pop from channel
*/

#ifndef POP_H
#define POP_H
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#include "../datatypes/channel_descriptor.h"


/*
 * ATTENTION: this depends on:
 *  - the internal routing table (internal_receiver_rt). It maps tag to channel_id.
 *      It must be available as a global constant char array
 *  - the array of channel channel_from_ck_r
 *
 *
 */

//TODO: understand where we should put this. Probably a code-generated header with the topology
extern __constant char internal_receiver_rt[2];
extern channel SMI_NetworkMessage channel_from_ck_r[2];

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
        const char chan_idx=internal_receiver_rt[chan->tag];
        chan->net=read_channel_intel(channel_from_ck_r[chan_idx]);
    }
    char * ptr=chan->net.data+(chan->packet_element_id)*chan->size_of_type;
    chan->packet_element_id++;                       //first increment and then use it: otherwise compiler detects Fmax problems
    //TODO: this prevents HyperFlex (try with a constant and you'll see)
    //I had to put this check, because otherwise II goes to 2
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

}

#endif //ifndef POP_

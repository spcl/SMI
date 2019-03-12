/**
    Generic unpacker: takes in input a network message and unpack it
    into multiple application messages (according to DATA_PER_PACKET).

    The body is constituted by a single loop


    It can be used as skeleton for specialized implementation.

    NOTE: this is a skeleton. Proper channel definition is missing.
*/

#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../datatypes/network_message.h"
#include "../datatypes/application_message.h"

//This defines and channels declaration are provisional
#define DATA_TYPE int       //type of application data
#define DATA_SIZE 4         //sizeof(DATA_TYPE)
#define DATA_PER_PACKET 6   //how many application data fits into a network message

channel app_message_t chan_from_unpacker;
channel network_message_t chan_from_ck_r;


__kernel void generic_unpacker()
{
    //each received packet must be produced as a set of integers
    int size=-1;
    int it=DATA_PER_PACKET-1;

    int sent=0;
    bool first=true;

    network_message_t m;
    while(1)
    {
        if(it==DATA_PER_PACKET-1)
        {
            it=-1;
            m=read_channel_intel(chan_from_ck_r);
            if(sent>size) //new message
            {
                size=m.header.size;
                sent=0;
            }
        }
        app_message_t app;
        char * ptr=m.data+(it)*DATA_SIZE;
        it++;                       //first increment and then use it: otherwise compiler detects Fmax problems
        sent++;
        //create packet
        app.header.size=m.header.size;
        app.header.src=m.header.src;
        app.header.dst=m.header.dst;
        DATA_TYPE *rcv=(DATA_TYPE*)(ptr);

        app.data=rcv[1];
        if(sent<=size) //in this way we are wasting some iterations if the size is not a multiple but Hyperflex is on
            write_channel_intel(chan_from_unpacker,app);

    }
}

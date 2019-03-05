/**
    Generic packer: takes in input messages from the application
    and pack them into a network message.

    NOTE: this is a skeleton. Proper channel definition is missing.
*/

#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../datatypes/network_message.h"
#include "../datatypes/application_message.h"

//This defines and channels declaration are provisional
#define DATA_TYPE int       //type of application data
#define DATA_SIZE 4         //sizeof(DATA_TYPE)
#define DATA_PER_PACKET 6   //how many application data fits into a network message

channel app_message_t chan_to_packer;
channel network_message_t chan_to_ck_s;


__kernel void generic_packer()
{
    //TODO: exit conditions
    network_message_t mess;
    bool first=true;
    int size=1;
    int data_id=0; //current number of data in the network packet

    //NOTE: for the moment being it is assumed that the application message
    //report the (total) message size
    while(1)
    {

        app_message_t app=read_channel_intel(chan_to_packer);
        size--; //decrement is here to avoid Fmax problems
        if(first)
            size=app.header.size-1;

        //create the message
        //copy the data
        char *conv=(char*)&app.data;
        #pragma unroll
        for(int jj=0;jj<DATA_SIZE;jj++)
            mess.data[data_id*DATA_SIZE+jj]=conv[jj];
        mess.header=app.header;
        first=(size==0);

        if(data_id==DATA_PER_PACKET-1 || first) //send it if packet is filled or we reached the message size
        {
            data_id=0;
            write_channel_intel(chan_to_ck_s,mess);
        }
        else
            data_id++;
    }
}

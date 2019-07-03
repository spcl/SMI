#ifndef NETWORK_MESSAGE_H
#define NETWORK_MESSAGE_H
#pragma OPENCL EXTENSION cl_intel_channels : enable
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

/**
 * Message sent over the network. 256 bits wide. It contains
 * - an array of char that will hold the actual data
 * - the header
 */

#include "header_message.h"

typedef struct __attribute__((packed)) __attribute__((aligned(32))){
    union{
        struct __attribute__((packed)) __attribute__((aligned(32))){
            char data[28];
            SMI_Message_header header;
        };
        char padding_[32];
    };
}SMI_Network_message;


#endif //ifndef NETWORK_MESSAGE_H

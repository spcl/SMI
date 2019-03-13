#ifndef NETWORK_MESSAGE_H
#define NETWORK_MESSAGE_H

/**
 * Message sent over the network. 256 bit wide. It contains
 * - an array of char that will hold the actual data
 * - the header
 */

#include "header_message.h"

typedef struct __attribute__((packed)) __attribute__((aligned(32))){
    union{
        struct __attribute__((packed)) __attribute__((aligned(32))){
            char data[28];
            header_t header;
        };
        char padding_[32];
    };
}network_message_t;


#endif //ifndef NETWORK_MESSAGE_H

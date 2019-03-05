#ifndef NETWORK_MESSAGE_H
#define NETWORK_MESSAGE_H

/**
 * Message sent over the network. 256 bit wide. It contains
 * - the header
 * - an array of char that will hold the actual data
 */

#include "header_message.h"

typedef struct __attribute__((packed)) __attribute__((aligned(16))){
    union{
        struct __attribute__((packed)) __attribute__((aligned(16))){
            header_t header;
            char data[24];
        };
        char padding_[32];
    };
}network_message_t;


#endif //ifndef NETWORK_MESSAGE_H

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



#define SMI_CHAR_TYPE_SIZE      1
#define SMI_SHORT_TYPE_SIZE     1
#define SMI_INT_TYPE_SIZE       4
#define SMI_FLOAT_TYPE_SIZE     4
#define SMI_DOUBLE_TYPE_SIZE    8

#define SMI_CHAR_ELEM_PER_PCKT      28
#define SMI_SHORT_ELEM_PER_PCKT     14
#define SMI_INT_ELEM_PER_PCKT       7
#define SMI_FLOAT_ELEM_PER_PCKT     7
#define SMI_DOUBLE_ELEM_PER_PCKT    3

/**
  Copies data from a given dest to the network message(NETM) into
  a given channel (CHAN)
*/
#define COPY_DATA_TO_NET_MESSAGE(CHAN,NETM,SRC) {\
    char *smi_macro_data_snd=CHAN->NETM.data;\
    switch(CHAN->data_type) /*copy the data*/\
    {\
        case SMI_CHAR:\
            _Pragma("unroll")\
            for (int ee = 0; ee < SMI_CHAR_ELEM_PER_PCKT; ee++) {\
                if (ee == CHAN->packet_element_id) {\
                     _Pragma("unroll")\
                    for (int jj = 0; jj < SMI_CHAR_TYPE_SIZE; jj++) {\
                        smi_macro_data_snd[(ee * SMI_CHAR_TYPE_SIZE) + jj] = SRC[jj];\
                    }\
                }\
            }\
        break;\
        case SMI_SHORT:\
            _Pragma("unroll")\
            for (int ee = 0; ee < SMI_SHORT_ELEM_PER_PCKT; ee++) {\
                if (ee == CHAN->packet_element_id) {\
                     _Pragma("unroll")\
                    for (int jj = 0; jj < SMI_SHORT_TYPE_SIZE; jj++) {\
                        smi_macro_data_snd[(ee * SMI_SHORT_TYPE_SIZE) + jj] = SRC[jj];\
                    }\
                }\
            }\
        break;\
        case SMI_INT:\
        case SMI_FLOAT:\
            _Pragma("unroll")\
            for (int ee = 0; ee < SMI_INT_ELEM_PER_PCKT; ee++) {\
                if (ee == CHAN->packet_element_id) {\
                     _Pragma("unroll")\
                    for (int jj = 0; jj < SMI_INT_TYPE_SIZE; jj++) {\
                        smi_macro_data_snd[(ee * SMI_INT_TYPE_SIZE) + jj] = SRC[jj];\
                    }\
                }\
            }\
        break;\
        case SMI_DOUBLE:\
            _Pragma("unroll")\
            for (int ee = 0; ee < SMI_DOUBLE_ELEM_PER_PCKT; ee++) {\
                if (ee == CHAN->packet_element_id) {\
                     _Pragma("unroll")\
                    for (int jj = 0; jj < SMI_DOUBLE_TYPE_SIZE; jj++) {\
                        smi_macro_data_snd[(ee * SMI_DOUBLE_TYPE_SIZE) + jj] = SRC[jj];\
                    }\
                }\
            }\
            break;\
    }\
}

/**
  Copies data from a network message(NETM) in a channel(CHAN)
    to a user target variable (DST)
*/
#define COPY_DATA_FROM_NET_MESSAGE(CHAN,NETM,DST) { \
    char *smi_macro_data_rcv=CHAN->NETM.data; \
    switch(CHAN->data_type) \
    { \
        case SMI_CHAR: \
            _Pragma("unroll")\
            for (int ee = 0; ee < SMI_CHAR_ELEM_PER_PCKT; ee++) { \
                if (ee == CHAN->packet_element_id_rcv) { \
                    _Pragma("unroll")\
                    for (int jj = 0; jj < SMI_CHAR_TYPE_SIZE; jj++) { \
                        ((char *)DST)[jj] = smi_macro_data_rcv[(ee * SMI_CHAR_TYPE_SIZE) + jj]; \
                    } \
                } \
            } \
            break; \
        case SMI_SHORT: \
            _Pragma("unroll")\
            for (int ee = 0; ee < SMI_SHORT_ELEM_PER_PCKT; ee++) { \
                if (ee == CHAN->packet_element_id_rcv) { \
                    _Pragma("unroll")\
                    for (int jj = 0; jj < SMI_SHORT_TYPE_SIZE; jj++) { \
                        ((char *)DST)[jj] = smi_macro_data_rcv[(ee * SMI_SHORT_TYPE_SIZE) + jj]; \
                    } \
                } \
            } \
        case SMI_INT: \
        case SMI_FLOAT: \
            _Pragma("unroll")\
            for (int ee = 0; ee < SMI_INT_ELEM_PER_PCKT; ee++) { \
                if (ee == CHAN->packet_element_id_rcv) { \
                    _Pragma("unroll")\
                    for (int jj = 0; jj < SMI_INT_TYPE_SIZE; jj++) { \
                        ((char *)DST)[jj] = smi_macro_data_rcv[(ee * SMI_INT_TYPE_SIZE) + jj]; \
                    } \
                } \
            } \
            break; \
        case SMI_DOUBLE: \
            _Pragma("unroll")\
            for (int ee = 0; ee < SMI_DOUBLE_ELEM_PER_PCKT; ee++) { \
                if (ee == CHAN->packet_element_id_rcv) { \
                    _Pragma("unroll")\
                    for (int jj = 0; jj < SMI_DOUBLE_TYPE_SIZE; jj++) { \
                        ((char *)DST)[jj] = smi_macro_data_rcv[(ee * SMI_DOUBLE_TYPE_SIZE) + jj]; \
                    } \
                } \
            } \
            break; \
    } \
}

#endif //ifndef NETWORK_MESSAGE_H

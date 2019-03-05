/**
 *
 * Generic application message. It contains:
 * - header (for the moment being the same used in network message)
 * - data: a single element of a given type
 *
 **/

#include "header_message.h"
#define DATA_TYPE int
typedef struct __attribute__((packed)) {
   header_t header;
   DATA_TYPE data;
}app_message_t;


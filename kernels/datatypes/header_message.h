#ifndef HEADER_MESSAGE_H
#define HEADER_MESSAGE_H
/**
    Message header definition
*/

//TODO: put all the info here

typedef struct __attribute__((packed)) {
    uint size;
    char src;
    char dst;
}header_t;

#endif //ifndef HEADER_MESSAGE_H

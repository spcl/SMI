/**
    Message header definition with macros for accessing it
    (for now they just take the corresponding field)
*/
#ifndef HEADER_MESSAGE_H
#define HEADER_MESSAGE_H

#define GET_HEADER_SRC(H) (H.src)
#define GET_HEADER_DST(H) (H.dst)
#define GET_HEADER_TAG(H) (H.tag)
#define GET_HEADER_OP(H) (H.op)
#define SET_HEADER_SRC(H,S) (H.src=S)
#define SET_HEADER_DST(H,D) (H.dst=D)
#define SET_HEADER_TAG(H,T) (H.tag=T)
#define SET_HEADER_OP(H,O) (H.op=O)


typedef struct __attribute__((packed)) {
    char src;
    char dst;
    char tag;
    char op; //defined as char to save space (enum is 4 byte long)

}SMI_MessageHeader;

#endif //ifndef HEADER_MESSAGE_H

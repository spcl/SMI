/**
    Message header definition with macros for accessing it
    (for now they just take the corresponding field)
*/
#ifndef HEADER_MESSAGE_H
#define HEADER_MESSAGE_H

#define GET_HEADER_SRC(H) (H.src)
#define GET_HEADER_DST(H) (H.dst)
#define GET_HEADER_PORT(H) (H.port)
#define GET_HEADER_OP(H) ((char)H.elems_and_op & (char)7)
#define GET_HEADER_NUM_ELEMS(H) ((char)H.elems_and_op >> ((char)3))   //returns the number of valid data elements in the packet
#define SET_HEADER_SRC(H,S) (H.src=S)
#define SET_HEADER_DST(H,D) (H.dst=D)
#define SET_HEADER_PORT(H,P) (H.port=P)
#define SET_HEADER_OP(H,O) (H.elems_and_op=((H.elems_and_op & 248) | O & 7))
#define SET_HEADER_NUM_ELEMS(H,N) (H.elems_and_op=((H.elems_and_op &7) | (N << 3))) //By assumption N < 32


typedef struct __attribute__((packed)) {
    char src;
    char dst;
    char port;
    char elems_and_op;    //upper 5 bits contain the number of valid data elements in the packet
                          //lower 3 bit contain the type of operation

}SMI_Message_header;

#endif //ifndef HEADER_MESSAGE_H

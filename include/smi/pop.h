/**
    Pop from channel
*/

#ifndef POP_H
#define POP_H
#include "channel_descriptor.h"
#include "communicator.h"


/**
 * @brief SMI_Open_receive_channel opens a receive transient channel
 * @param count number of data elements to receive
 * @param data_type data type of the data elements
 * @param source rank of the sender
 * @param port port number
 * @param comm communicator
 * @return channel descriptor
 */
SMI_Channel SMI_Open_receive_channel(int count, SMI_Datatype data_type, int source, int port, SMI_Comm comm)
{
    // implemented in codegen
    SMI_Channel chan;
    return chan;
}
SMI_Channel SMI_Open_receive_channel_ad(int count, SMI_Datatype data_type, int source, int port, SMI_Comm comm, int buffer_size)
{
    // fake function
    SMI_Channel chan;
    return chan;
}

/**
 * @brief SMI_Pop: receive a data element. Returns only when data arrives
 * @param chan pointer to the transient channel descriptor
 * @param data pointer to the target variable that, on return, will contain the data element
 */
void SMI_Pop(SMI_Channel *chan, void *data)
{
    // implemented in codegen
}

#endif //ifndef POP_H

/**
    Push to channel
*/

#ifndef PUSH_H
#define PUSH_H
#include "channel_descriptor.h"
#include "communicator.h"

/**
 * @brief SMI_OpenSendChannel
 * @param count number of data elements to send
 * @param data_type type of the data element
 * @param destination rank of the destination
 * @param port port number
 * @param comm communicator
 * @return channel descriptor
 */
SMI_Channel SMI_Open_send_channel(int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm);
SMI_Channel SMI_Open_send_channel_ad(int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm, int bufferSize);

/**
 * @brief private function SMI_Push push a data elements in the transient channel. Data transferring can be delayed
 * @param chan
 * @param data
 * @param immediate: if true the data is immediately sent, without waiting for the completion of the network packet.
 *          In general, the user should use the other Push definition
 */
void SMI_Push_flush(SMI_Channel *chan, void* data, int immediate);

/**
 * @brief SMI_Push push a data elements in the transient channel. The actual ata transferring can be delayed
 * @param chan pointer to the channel descriptor of the transient channel
 * @param data pointer to the data that can be sent
 */
void SMI_Push(SMI_Channel *chan, void* data);

#endif //ifndef PUSH_H

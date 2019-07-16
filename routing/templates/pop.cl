{% import 'utils.cl' as utils %}

{% macro smi_pop(program, op) -%}
/**
 * @brief SMI_Pop: receive a data element. Returns only when data arrives
 * @param chan pointer to the transient channel descriptor
 * @param data pointer to the target variable that, on return, will contain the data element
 */
void {{ utils.impl_name_port_type("SMI_Pop", op) }}(SMI_Channel *chan, void *data)
{
    {% set ckr_data = program.create_group("ckr_data") %}
    {% set cks_control = program.create_group("cks_control") %}

    // in this case we have to copy the data into the target variable
    if (chan->packet_element_id == 0)
    {
        // no data to be unpacked...receive from the network
        chan->net = read_channel_intel({{ utils.channel_array("ckr_data")}}[{{ ckr_data.get_hw_port(op.logical_port) }}]);
    }
    chan->processed_elements++;
    char *data_recvd = chan->net.data;

    #pragma unroll
    for (int ee = 0; ee < {{ op.data_elements_per_packet() }}; ee++)
    {
        if (ee == chan->packet_element_id)
        {
            #pragma unroll
            for (int jj = 0; jj < {{ op.data_size() }}; jj++)
            {
                ((char *)data)[jj] = data_recvd[(ee * {{ op.data_size() }}) + jj];
            }
        }
    }

    chan->packet_element_id++;
    if (chan->packet_element_id == GET_HEADER_NUM_ELEMS(chan->net.header))
    {
        chan->packet_element_id = 0;
    }
    chan->tokens--;
    // TODO: This is used to prevent this funny compiler to re-oder the two *_channel_intel operations
    // mem_fence(CLK_CHANNEL_MEM_FENCE);

    if (chan->tokens == 0)
    {
        // At this point, the sender has still max_tokens*7/8 tokens: we have to consider this while we send
        // the new tokens to it
        unsigned int sender = ((int) ((int) chan->message_size - (int) chan->processed_elements - (int) chan->max_tokens * 7 / 8)) < 0 ? 0: chan->message_size - chan->processed_elements - chan -> max_tokens * 7 / 8;
        chan->tokens = (unsigned int) (MIN(chan->max_tokens / 8, sender)); // b/2
        SMI_Network_message mess;
        *(unsigned int*) mess.data = chan->tokens;
        SET_HEADER_DST(mess.header, chan->sender_rank);
        SET_HEADER_PORT(mess.header, chan->port);
        SET_HEADER_OP(mess.header, SMI_SYNCH);
        write_channel_intel({{ utils.channel_array("cks_control")}}[{{ cks_control.get_hw_port(op.logical_port) }}], mess);
    }
}
{% endmacro %}

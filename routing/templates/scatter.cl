{% import 'utils.cl' as utils %}

{% macro smi_scatter_kernel(program, op) -%}
__kernel void smi_kernel_scatter_{{ op.logical_port }}(char num_rank)
{
    bool external = true;
    char to_be_received_requests = 0; // how many ranks have still to communicate that they are ready to receive
    const char num_requests = num_rank - 1;
    SMI_Network_message mess;
    {% set ckr_control = program.create_group("ckr_control") %}
    {% set cks_data = program.create_group("cks_data") %}
    {% set scatter = program.create_group("scatter") %}

    while (true)
    {
        if (external) // read from the application
        {
            mess = read_channel_intel({{ utils.channel_array("scatter") }}[{{ scatter.get_hw_port(op.logical_port) }}]);
            if (GET_HEADER_OP(mess.header) == SMI_SYNCH)
            {
                to_be_received_requests = num_requests;
                SET_HEADER_OP(mess.header, SMI_SCATTER);
            }
            external = false;
        }
        else // handle the request
        {
            if (to_be_received_requests != 0)
            {
                SMI_Network_message req = read_channel_intel({{ utils.channel_array("ckr_control") }}[{{ ckr_control.get_hw_port(op.logical_port) }}]);
                to_be_received_requests--;
            }
            else
            {
                //just push it to the network
                write_channel_intel({{ utils.channel_array("cks_data") }}[{{ cks_data.get_hw_port(op.logical_port) }}], mess);
                external = true;
            }
        }
    }
}
{%- endmacro %}

{% macro smi_scatter_impl(program, op) -%}
/**
 * @brief SMI_Scatter
 * @param chan pointer to the scatter channel descriptor
 * @param data_snd pointer to the data element that must be sent (root only)
 * @param data_rcv pointer to the receiving data element
 */
void {{ utils.impl_name_port_type("SMI_Scatter", op) }}(SMI_ScatterChannel* chan, void* data_snd, void* data_rcv)
{
    // take here the pointers to send/recv data to avoid fake dependencies
    const char elem_per_packet = chan->elements_per_packet;
    if (chan->my_rank == chan->root_rank) // I'm the root
    {
        // the root is responsible for splitting the data in packets
        // and set the right receviver.
        // If the receiver is itself it has to set the data_rcv accordingly
        char* conv = (char*) data_snd;
        const unsigned int message_size = chan->send_count;
        chan->processed_elements++;

        char* data_to_send = chan->net.data;

        #pragma unroll
        for (int ee = 0; ee < {{ op.data_elements_per_packet() }}; ee++)
        {
            if (ee == chan->packet_element_id)
            {
                #pragma unroll
                for (int jj = 0; jj < {{ op.data_size() }}; jj++)
                {
                    if (chan->next_rcv == chan->my_rank)
                    {
                        ((char*) (data_rcv))[jj] = conv[jj];
                    }
                    else data_to_send[(ee * {{ op.data_size() }}) + jj] = conv[jj];
                }
            }
        }

{% set scatter = program.create_group("scatter") %}
{% set ckr_data = program.create_group("ckr_data") %}
{% set cks_control = program.create_group("cks_control") %}

        chan->packet_element_id++;
        // split this in packets holding send_count elements
        if (chan->packet_element_id == elem_per_packet ||
            chan->processed_elements == message_size) // send it if packet is filled or we reached the message size
        {
            SET_HEADER_NUM_ELEMS(chan->net.header, chan->packet_element_id);
            SET_HEADER_PORT(chan->net.header, chan->port);
            SET_HEADER_DST(chan->net.header, chan->next_rcv);
            // offload to scatter kernel

            if (chan->next_rcv != chan->my_rank)
            {
                write_channel_intel({{ utils.channel_array("scatter") }}[{{ scatter.get_hw_port(op.logical_port) }}], chan->net);
                SET_HEADER_OP(chan->net.header, SMI_SCATTER);
            }

            chan->packet_element_id = 0;
            if (chan->processed_elements == message_size)
            {
                // we finished the data that need to be sent to this receiver
                chan->processed_elements = 0;
                chan->next_rcv++;
            }
        }
    }
    else // non-root rank: receive and unpack
    {
        if(chan->init)  //send ready-to-receive to the root
        {
            write_channel_intel({{ utils.channel_array("cks_control") }}[{{ cks_control.get_hw_port(op.logical_port) }}], chan->net);
            chan->init=false;
        }
        if (chan->packet_element_id_rcv == 0)
        {
            chan->net_2 = read_channel_intel({{ utils.channel_array("ckr_data") }}[{{ ckr_data.get_hw_port(op.logical_port) }}]);
        }
        COPY_DATA_FROM_NET_MESSAGE(chan, chan->net_2, data_rcv);

        chan->packet_element_id_rcv++;
        if (chan->packet_element_id_rcv == elem_per_packet)
        {
            chan->packet_element_id_rcv = 0;
        }
    }
}
{%- endmacro %}

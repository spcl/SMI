{% import 'utils.cl' as utils %}

{% macro smi_gather_kernel(program, op) -%}
__kernel void smi_kernel_gather_{{ op.logical_port }}(char num_rank)
{
    // receives the data from the application and
    // forwards it to the root only when the SYNCH message arrives
    SMI_Network_message mess;
    {% set ckr_control = program.create_group("ckr_control") %}
    {% set cks_data = program.create_group("cks_data") %}
    {% set gather = program.create_group("gather") %}

    while (true)
    {
        mess = read_channel_intel({{ utils.channel_array("gather") }}[{{ gather.get_hw_port(op.logical_port) }}]);
        if (GET_HEADER_OP(mess.header) == SMI_SYNCH)
        {
            SMI_Network_message req = read_channel_intel({{ utils.channel_array("ckr_control") }}[{{ ckr_control.get_hw_port(op.logical_port) }}]);
        }
        // TODO: understand how to enable this without incurring in II penalties
        // this is necessary for correctness to guarantee the ordering of channel operations
        // mem_fence(CLK_CHANNEL_MEM_FENCE);
        SET_HEADER_NUM_ELEMS(mess.header, MAX(GET_HEADER_NUM_ELEMS(mess.header), GET_HEADER_NUM_ELEMS(req.header)));

        SET_HEADER_OP(mess.header, SMI_GATHER);
        write_channel_intel({{ utils.channel_array("cks_data") }}[{{ cks_data.get_hw_port(op.logical_port) }}], mess);
    }
}
{%- endmacro %}

{% macro smi_gather_impl(program, op) -%}
/**
 * @brief SMI_Gather
 * @param chan pointer to the gather channel descriptor
 * @param data_snd pointer to the data element that must be sent
 * @param data_rcv pointer to the receiving data element (significant on the root rank only)
 */
void {{ utils.impl_name_port_type("SMI_Gather", op) }}(SMI_GatherChannel* chan, void* send_data, void* rcv_data)
{
{% set gather = program.create_group("gather") %}
{% set cks_control = program.create_group("cks_control") %}
{% set ckr_data = program.create_group("ckr_data") %}

    if (chan->my_rank == chan->root_rank) // I'm the root
    {
        // we can't offload this part to the kernel, by passing a network message
        // because it will replies to the root once every elem_per_packet cycles
        // the root is responsible for receiving data from a contributor, after sending it a request
        // for sending the data. If the contributor is itself it has to set the rcv_data accordingly
        const int message_size = chan->recv_count;

        if (chan->next_contrib != chan->my_rank && chan->processed_elements_root == 0 ) // at the beginning we have to send the request
        {
            // SMI_Network_message request;
            SET_HEADER_OP(chan->net_2.header, SMI_SYNCH);
            SET_HEADER_NUM_ELEMS(chan->net_2.header, 1); // this is used in the support kernel
            SET_HEADER_DST(chan->net_2.header, chan->next_contrib);
            SET_HEADER_PORT(chan->net_2.header, {{ op.logical_port }});
            write_channel_intel({{ utils.channel_array("cks_control") }}[{{ cks_control.get_hw_port(op.logical_port) }}], chan->net_2);
        }
        // This fence is not necessary, the two channel operation are independent
        // and we don't need any ordering between them
        // mem_fence(CLK_CHANNEL_MEM_FENCE);

        // receive the data
        if (chan->packet_element_id_rcv == 0 && chan->next_contrib != chan->my_rank)
        {
            chan->net = read_channel_intel({{ utils.channel_array("ckr_data") }}[{{ ckr_data.get_hw_port(op.logical_port) }}]);
        }

        char* data_recvd = chan->net.data;
        char* conv = (char*) send_data;

        #pragma unroll
        for (int ee = 0; ee < {{ op.data_elements_per_packet() }}; ee++)
        {
            if (ee == chan->packet_element_id_rcv)
            {
                #pragma unroll
                for (int jj = 0; jj < {{ op.data_size() }}; jj++)
                {
                    if (chan->next_contrib != chan->root_rank)     // not my turn
                    {
                        ((char *)rcv_data)[jj] = data_recvd[(ee * {{ op.data_size() }}) + jj];
                    }
                    else ((char *)rcv_data)[jj] = conv[jj];
                }
            }
        }

        chan->processed_elements_root++;
        chan->packet_element_id_rcv++;
        if (chan->packet_element_id_rcv == chan->elements_per_packet)
        {
            chan->packet_element_id_rcv = 0;
        }

        if (chan->processed_elements_root == message_size)
        {
            // we finished the data that need to be received from this contributor, go to the next one
            chan->processed_elements_root = 0;
            chan->next_contrib++;
            chan->packet_element_id_rcv = 0;
        }
    }
    else
    {
        // Non root rank, pack the data and send it
        char* conv = (char*) send_data;
        const int message_size = chan->send_count;
        chan->processed_elements++;
        // copy the data (compiler fails with the macro)
        // COPY_DATA_TO_NET_MESSAGE(chan,chan->net,conv);
        char* data_snd = chan->net.data;

        #pragma unroll
        for (char jj = 0; jj < {{ op.data_size() }}; jj++)
        {
            data_snd[chan->packet_element_id * {{ op.data_size() }} + jj] = conv[jj];
        }
        chan->packet_element_id++;

        if (chan->packet_element_id == chan->elements_per_packet || chan->processed_elements == message_size)
        {
            SET_HEADER_NUM_ELEMS(chan->net.header, chan->packet_element_id);
            SET_HEADER_DST(chan->net.header, chan->root_rank);

            write_channel_intel({{ utils.channel_array("gather") }}[{{ gather.get_hw_port(op.logical_port) }}], chan->net);
            // first one is a SMI_SYNCH, used to communicate to the support kernel that it has to wait for a "ready to receive" from the root
            SET_HEADER_OP(chan->net.header, SMI_GATHER);
            chan->packet_element_id = 0;
        }
    }
}
{%- endmacro %}

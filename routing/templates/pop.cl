{% import 'utils.cl' as utils %}

{%- macro smi_pop_impl(program, op) -%}
void {{ utils.impl_name_port_type("SMI_Pop", op) }}(SMI_Channel *chan, void *data)
{
    // in this case we have to copy the data into the target variable
    if (chan->packet_element_id == 0)
    {
        // no data to be unpacked...receive from the network
        chan->net = read_channel_intel({{ op.get_channel("ckr_data") }});
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
        write_channel_intel({{ op.get_channel("cks_control") }}, mess);
    }
}
{%- endmacro %}

{%- macro smi_pop_channel(program, op) -%}
SMI_Channel {{ utils.impl_name_port_type("SMI_Open_receive_channel", op) }}(int count, SMI_Datatype data_type, int source, int port, SMI_Comm comm)
{
    SMI_Channel chan;
    // setup channel descriptor
    chan.port = (char) port;
    chan.sender_rank = (char) source;
    chan.message_size = (unsigned int) count;
    chan.data_type = data_type;
    chan.op_type = SMI_RECEIVE;
    chan.elements_per_packet = {{ op.data_elements_per_packet() }};
    chan.max_tokens = {{ op.buffer_size * op.data_elements_per_packet() }};

#if defined P2P_RENDEZVOUS
    chan.tokens = MIN(chan.max_tokens / ((unsigned int) 8), count); // needed to prevent the compiler to optimize-away channel connections
#else
    chan.tokens = count; // in this way, the last rendezvous is done at the end of the message. This is needed to prevent the compiler to cut-away internal FIFO buffer connections
#endif
    // The receiver sends tokens to the sender once every chan.max_tokens/8 received data elements
    // chan.tokens = chan.max_tokens / ((unsigned int) 8);
    SET_HEADER_NUM_ELEMS(chan.net.header, 0);    // at the beginning no data
    chan.packet_element_id = 0; // data per packet
    chan.processed_elements = 0;
    chan.sender_rank = chan.sender_rank;
    chan.receiver_rank = comm[0];
    // comm is not directly used in this first implementation
    return chan;
}
{%- endmacro -%}

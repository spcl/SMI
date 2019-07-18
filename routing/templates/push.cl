{% import 'utils.cl' as utils %}

{%- macro smi_push_impl(program, op) -%}
void {{ utils.impl_name_port_type("SMI_Push_flush", op) }}(SMI_Channel *chan, void* data, int immediate)
{
    char* conv = (char*) data;
    COPY_DATA_TO_NET_MESSAGE(chan, chan->net, conv);
    chan->processed_elements++;
    chan->packet_element_id++;
    chan->tokens--;

    // send the network packet if it full or we reached the message size
    if (chan->packet_element_id == chan->elements_per_packet || immediate || chan->processed_elements == chan->message_size)
    {
        SET_HEADER_NUM_ELEMS(chan->net.header, chan->packet_element_id);
        chan->packet_element_id = 0;
        write_channel_intel({{ op.get_channel("cks_data") }}, chan->net);
    }
    // This fence is not mandatory, the two channel operations can be
    // performed independently
    // mem_fence(CLK_CHANNEL_MEM_FENCE);

    if (chan->tokens == 0)
    {
        // receives also with tokens=0
        // wait until the message arrives
        SMI_Network_message mess = read_channel_intel({{ op.get_channel("ckr_control") }});
        unsigned int tokens = *(unsigned int *) mess.data;
        chan->tokens += tokens; // tokens
    }
}
void {{ utils.impl_name_port_type("SMI_Push", op) }}(SMI_Channel *chan, void* data)
{
    {{ utils.impl_name_port_type("SMI_Push_flush", op) }}(chan, data, 0);
}
{%- endmacro %}

{%- macro smi_push_channel(program, op) -%}
SMI_Channel {{ utils.impl_name_port_type("SMI_Open_send_channel", op) }}(int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm)
{
    SMI_Channel chan;
    // setup channel descriptor
    chan.port = (char) port;
    chan.message_size = (unsigned int) count;
    chan.data_type = data_type;
    chan.op_type = SMI_SEND;
    chan.receiver_rank = (char) destination;
    // At the beginning, the sender can sends as many data items as the buffer size
    // in the receiver allows
    chan.elements_per_packet = {{ op.data_elements_per_packet() }};
    chan.max_tokens = {{ op.buffer_size * op.data_elements_per_packet() }};

    // setup header for the message
    SET_HEADER_DST(chan.net.header, chan.receiver_rank);
    SET_HEADER_PORT(chan.net.header, chan.port);
    SET_HEADER_OP(chan.net.header, SMI_SEND);
#if defined P2P_RENDEZVOUS
    chan.tokens = MIN(chan.max_tokens, count); // needed to prevent the compiler to optimize-away channel connections
#else // eager transmission protocol
    chan.tokens = count;  // in this way, the last rendezvous is done at the end of the message. This is needed to prevent the compiler to cut-away internal FIFO buffer connections
#endif
    chan.receiver_rank = destination;
    chan.processed_elements = 0;
    chan.packet_element_id = 0;
    chan.sender_rank = comm[0];
    // chan.comm = comm; // comm is not used in this first implemenation
    return chan;
}
{%- endmacro -%}

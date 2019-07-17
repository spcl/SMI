{% import 'utils.cl' as utils %}

{%- macro smi_bcast_kernel(program, op) -%}
__kernel void smi_kernel_bcast_{{ op.logical_port }}(char num_rank)
{
    bool external = true;
    char rcv;
    char root;
    char received_request = 0; // how many ranks are ready to receive
    const char num_requests = num_rank - 1;
    SMI_Network_message mess;
    {% set ckr_control = program.create_group("ckr_control") %}
    {% set cks_data = program.create_group("cks_data") %}
    {% set broadcast = program.create_group("broadcast") %}

    while (true)
    {
        if (external) // read from the application
        {
            mess = read_channel_intel({{ utils.channel_array("broadcast") }}[{{ broadcast.get_hw_port(op.logical_port) }}]);
            if (GET_HEADER_OP(mess.header) == SMI_SYNCH)   // beginning of a broadcast, we have to wait for "ready to receive"
            {
                received_request = num_requests;
            }
            SET_HEADER_OP(mess.header, SMI_BROADCAST);
            rcv = 0;
            external = false;
            root = GET_HEADER_SRC(mess.header);
        }
        else // handle the request
        {
            if (received_request != 0)
            {
                SMI_Network_message req = read_channel_intel({{ utils.channel_array("ckr_control") }}[{{ ckr_control.get_hw_port(op.logical_port) }}]);
                received_request--;
            }
            else
            {
                if (rcv != root) // it's not me
                {
                    SET_HEADER_DST(mess.header, rcv);
                    SET_HEADER_PORT(mess.header, {{ op.logical_port }});
                    write_channel_intel({{ utils.channel_array("cks_data") }}[{{ cks_data.get_hw_port(op.logical_port) }}], mess);
                }
                rcv++;
                external = rcv == num_rank;
            }
        }
    }
}
{%- endmacro %}

{%- macro smi_bcast_impl(program, op) -%}
void {{ utils.impl_name_port_type("SMI_Bcast", op) }}(SMI_BChannel* chan, void* data)
{
{% set broadcast = program.create_group("broadcast") %}
{% set ckr_data = program.create_group("ckr_data") %}
{% set cks_control = program.create_group("cks_control") %}
    char* conv = (char*)data;
    if (chan->my_rank == chan->root_rank) // I'm the root
    {
        const unsigned int message_size = chan->message_size;
        chan->processed_elements++;
        COPY_DATA_TO_NET_MESSAGE(chan, chan->net, conv);

        chan->packet_element_id++;
        // send the network packet if it is full or we reached the message size
        if (chan->packet_element_id == chan->elements_per_packet || chan->processed_elements == message_size)
        {
            SET_HEADER_NUM_ELEMS(chan->net.header, chan->packet_element_id);
            SET_HEADER_PORT(chan->net.header, {{ op.logical_port }});
            chan->packet_element_id = 0;

            // offload to support kernel
            write_channel_intel({{ utils.channel_array("broadcast") }}[{{ broadcast.get_hw_port(op.logical_port) }}], chan->net);
            SET_HEADER_OP(chan->net.header, SMI_BROADCAST);  // for the subsequent network packets
        }
    }
    else // I have to receive
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

        COPY_DATA_FROM_NET_MESSAGE(chan, chan->net_2, conv);

        chan->packet_element_id_rcv++;
        if (chan->packet_element_id_rcv == chan->elements_per_packet)
        {
            chan->packet_element_id_rcv = 0;
        }
    }
}
{%- endmacro %}

{%- macro smi_bcast_channel(program, op) -%}
SMI_BChannel {{ utils.impl_name_port_type("SMI_Open_bcast_channel", op) }}(int count, SMI_Datatype data_type, int port, int root, SMI_Comm comm)
{
    SMI_BChannel chan;
    // setup channel descriptor
    chan.message_size = count;
    chan.data_type = data_type;
    chan.port = (char) port;
    chan.my_rank = (char) SMI_Comm_rank(comm);
    chan.root_rank = (char) root;
    chan.num_rank = (char) SMI_Comm_size(comm);
    chan.init = true;
    chan.size_of_type = {{ op.data_size() }};
    chan.elements_per_packet = {{ op.data_elements_per_packet() }};

    if (chan.my_rank != chan.root_rank)
    {
        // At the beginning, send a "ready to receive" to the root
        // This is needed to not inter-mix subsequent collectives
        SET_HEADER_OP(chan.net.header, SMI_SYNCH);
        SET_HEADER_DST(chan.net.header, chan.root_rank);
        SET_HEADER_SRC(chan.net.header, chan.my_rank);
        SET_HEADER_PORT(chan.net.header, chan.port);
    }
    else
    {
        SET_HEADER_OP(chan.net.header, SMI_SYNCH);           // used to signal to the support kernel that a new broadcast has begun
        SET_HEADER_SRC(chan.net.header, chan.root_rank);
        SET_HEADER_PORT(chan.net.header, chan.port);         // used by destination
        SET_HEADER_NUM_ELEMS(chan.net.header, 0);            // at the beginning no data
    }
    chan.processed_elements = 0;
    chan.packet_element_id = 0;
    chan.packet_element_id_rcv = 0;
    return chan;
{%- endmacro -%}

{% import 'utils.cl' as utils %}

{%- macro smi_scatter_kernel(program, op) -%}
__kernel void smi_kernel_scatter_{{ op.logical_port }}(char num_rank)
{
    bool external = true;
    char to_be_received_requests = 0; // how many ranks have still to communicate that they are ready to receive
    const char num_requests = num_rank - 1;
    SMI_Network_message mess;

    while (true)
    {
        if (external) // read from the application
        {
            mess = read_channel_intel({{ op.get_channel("scatter") }});
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
                SMI_Network_message req = read_channel_intel({{ op.get_channel("ckr_control") }});
                to_be_received_requests--;
            }
            else
            {
                // just push it to the network
                write_channel_intel({{ op.get_channel("cks_data") }}, mess);
                external = true;
            }
        }
    }
}
{%- endmacro %}

{%- macro smi_scatter_impl(program, op) -%}
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
                write_channel_intel({{ op.get_channel("scatter") }}, chan->net);
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
            write_channel_intel({{ op.get_channel("cks_control") }}, chan->net);
            chan->init=false;
        }
        if (chan->packet_element_id_rcv == 0)
        {
            chan->net_2 = read_channel_intel({{ op.get_channel("ckr_data") }});
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

{%- macro smi_scatter_channel(program, op) -%}
SMI_ScatterChannel {{ utils.impl_name_port_type("SMI_Open_scatter_channel", op) }}(int send_count, int recv_count,
        SMI_Datatype data_type, int port, int root, SMI_Comm comm)
{
    SMI_ScatterChannel chan;
    // setup channel descriptor
    chan.send_count = (unsigned int) send_count;
    chan.recv_count = (unsigned int) recv_count;
    chan.data_type = data_type;
    chan.port = (char) port;
    chan.my_rank = (char) comm[0];
    chan.num_ranks = (char) comm[1];
    chan.root_rank = (char) root;
    chan.next_rcv = 0;
    chan.init = true;
    chan.size_of_type = {{ op.data_size() }};
    chan.elements_per_packet = {{ op.data_elements_per_packet() }};

    // setup header for the message
    if (chan.my_rank != chan.root_rank)
    {
        // this is set up to send a "ready to receive" to the root
        SET_HEADER_OP(chan.net.header, SMI_SYNCH);
        SET_HEADER_DST(chan.net.header, chan.root_rank);
        SET_HEADER_PORT(chan.net.header, chan.port);
        SET_HEADER_SRC(chan.net.header, chan.my_rank);
    }
    else
    {
        SET_HEADER_SRC(chan.net.header, chan.root_rank);
        SET_HEADER_PORT(chan.net.header, chan.port);         // used by destination
        SET_HEADER_NUM_ELEMS(chan.net.header, 0);            // at the beginning no data
        SET_HEADER_OP(chan.net.header, SMI_SYNCH);
    }

    chan.processed_elements = 0;
    chan.packet_element_id = 0;
    chan.packet_element_id_rcv = 0;
    return chan;
}
{%- endmacro -%}

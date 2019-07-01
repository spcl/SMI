{% import 'utils.cl' as utils %}

{% macro smi_scatter(program, op) -%}
__kernel void smi_kernel_scatter_{{ op.logical_port }}(char num_rank)
{
    bool external=true;
    char to_be_received_requests=0; //how many ranks have still to communicate that they are ready to receive
    const char num_requests=num_rank-1;
    SMI_Network_message mess;
    {% set ckr_control = program.create_group("ckr_control") %}
    {% set cks_data = program.create_group("cks_data") %}
    {% set scatter = program.create_group("scatter") %}

    while (true)
    {
        if (external) // read from the application
        {
            mess = read_channel_intel({{ utils.channel_array("scatter") }}[{{ scatter.get_hw_port(op.logical_port) }}]);
            if(GET_HEADER_OP(mess.header)==SMI_SYNCH)
                to_be_received_requests=num_requests;
            external=false;
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
                external=true;
            }
        }
    }
}
{%- endmacro %}


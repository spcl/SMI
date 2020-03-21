{% import 'utils.cl' as utils %}

{%- macro smi_cks(program, channel, channel_count, target_index) -%}
__kernel void smi_kernel_cks_{{ channel.index }}(__global volatile char *restrict rt, const char num_ranks)
{
    char external_routing_table[MAX_RANKS];
    for (int i = 0; i < MAX_RANKS; i++)
    {
        if (i < num_ranks)
        {
            external_routing_table[i] = rt[i];
        }
    }

{% set allocations = program.get_channel_allocations_with_prefix(channel.index, "cks") %}
    // number of CK_S - 1 + CK_R + {{ allocations|length }} CKS hardware ports
    const char num_sender = {{ channel_count + allocations|length }};
    char sender_id = 0;
    SMI_Network_message message;

    char contiguous_reads = 0;

    while (1)
    {
        bool valid = false;
        switch (sender_id)
        {
            {% for ck_s in channel.neighbours() %}
            case {{ loop.index0 }}:
                // receive from CK_S_{{ ck_s }}
                message = read_channel_nb_intel(channels_interconnect_ck_s[{{ (channel_count - 1) * channel.index + loop.index0 }}], &valid);
                break;
            {% endfor %}
            case {{ channel_count - 1 }}:
                // receive from CK_R_{{ channel.index }}
                message = read_channel_nb_intel(channels_interconnect_ck_r_to_ck_s[{{ channel.index }}], &valid);
                break;
            {% for (op, key) in allocations %}
            case {{ channel_count + loop.index0 }}:
                // receive from {{ op }}
                message = read_channel_nb_intel({{ op.get_channel(key) }}, &valid);
                break;
            {% endfor %}
        }

        if (valid)
        {
            // next time read from the next sender if we reach the max contiguous reader
            sender_id = (contiguous_reads == READS_LIMIT - 1) ? ((sender_id == num_sender-1) ? 0 :sender_id +1) : sender_id;
            // increase contiguous reads and reset if necessary
            contiguous_reads = (contiguous_reads == READS_LIMIT - 1)?0:contiguous_reads+1;
            char idx = external_routing_table[GET_HEADER_DST(message.header)];
            switch (idx)
            {
                case 0:
                    // send to QSFP
                    write_channel_intel(io_out_{{ channel.index }}, message);
                    break;
                case 1:
                    // send to CK_R_{{ channel.index }}
                    write_channel_intel(channels_interconnect_ck_s_to_ck_r[{{ channel.index }}], message);
                    break;
                {% for ck_s in channel.neighbours() %}
                case {{ 2 + loop.index0 }}:
                    // send to CK_S_{{ ck_s }}
                    write_channel_intel(channels_interconnect_ck_s[{{ (channel_count - 1) * ck_s + target_index(ck_s, channel.index) }}], message);
                    break;
                {% endfor %}
            }
        }
        else{
            //go to the next sender
            sender_id = (sender_id == num_sender-1) ? 0 : sender_id + 1;
            contiguous_reads = 0;
        }
    }
}
{%- endmacro %}

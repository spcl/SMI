{% macro cks(channel, channel_count, target_index) -%}
__kernel void CK_S_{{ channel.index }}(__global volatile char *restrict rt)
{
    char external_routing_table[RANK_COUNT];
    for (int i = 0; i < RANK_COUNT; i++)
    {
        external_routing_table[i] = rt[i];
    }

    // number of CK_S - 1 + CK_R + {{ channel.tags|length }} tags
    const char num_sender = {{ channel_count + channel.tags|length }};
    char sender_id = 0;
    SMI_Network_message message;

    const int READS_LIMIT=4;
    char contiguos_reads=0;
    while(1)
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
            {% for tag in channel.tags %}
            case {{ channel_count + loop.index0 }}:
                // receive from app channel with tag {{ tag }}
                message = read_channel_nb_intel(channels_to_ck_s[{{ tag }}], &valid);
                break;
            {% endfor %}
        }

        if (valid)
        {
            contiguos_reads++;
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
        if(!valid || contiguos_reads==READS_LIMIT)
        {
            contiguos_reads=0;
            sender_id++;
            if (sender_id == num_sender)
            {
                sender_id = 0;
            }
        }
    }
}
{%- endmacro %}

{% macro ckr(channel, channel_count, target_index, tag_count) -%}
__kernel void CK_R_{{ channel.index }}(__global volatile char *restrict rt, const char rank)
{
    char external_routing_table[{{ tag_count }} /* tag count */];
    for (int i = 0; i < {{ tag_count }} /* tag count */; i++)
    {
        external_routing_table[i] = rt[i];
    }

    // QSFP + number of CK_Rs - 1 + CK_S
    const char num_sender = {{ channel_count + 1 }};
    char sender_id = 0;
    SMI_Network_message message;
    const int READS_LIMIT=4;
    char contiguos_reads=0;
    while(1)
    {
        bool valid = false;
        switch (sender_id)
        {
            case 0:
                // QSFP
                message = read_channel_nb_intel(io_in_{{ channel.index }}, &valid);
                break;
            {% for ck_r in channel.neighbours() %}
            case {{ loop.index0 + 1 }}:
                // receive from CK_R_{{ ck_r }}
                message = read_channel_nb_intel(channels_interconnect_ck_r[{{ (channel_count - 1) * channel.index + loop.index0 }}], &valid);
                break;
            {% endfor %}
            case {{ channel_count }}:
                // receive from CK_S_{{ channel.index }}
                message = read_channel_nb_intel(channels_interconnect_ck_s_to_ck_r[{{ channel.index }}], &valid);
                break;
        }

        if (valid)
        {
            contiguos_reads++;
            char dest;
            if (GET_HEADER_DST(message.header) != rank)
            {
                dest = 0;
            }
            else dest = external_routing_table[GET_HEADER_TAG(message.header)];
            switch (dest)
            {
                case 0:
                    // send to CK_S_{{ channel.index }}
                    write_channel_intel(channels_interconnect_ck_r_to_ck_s[{{ channel.index }}], message);
                    break;
                {% for ck_r in channel.neighbours() %}
                case {{ loop.index0 + 1 }}:
                    // send to CK_R_{{ ck_r }}
                    write_channel_intel(channels_interconnect_ck_r[{{ (channel_count - 1) * ck_r + target_index(ck_r, channel.index) }}], message);
                    break;
                {% endfor %}
                {% for tag in channel.tags %}
                case {{ channel_count + loop.index0 }}:
                    // send to app channel with tag {{ tag }}
                    write_channel_intel(channels_from_ck_r[internal_receiver_rt[{{ tag }}]], message);
                    break;
                {% endfor %}
            }
        }

       if(!valid || contiguos_reads==READS_LIMIT)
        {
            contiguos_reads=0;
            sender_id++;
            if (sender_id == num_sender)
            {
                sender_id = 0;
            }
        }
    }
}
{%- endmacro %}

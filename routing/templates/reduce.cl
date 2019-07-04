{% import 'utils.cl' as utils %}

{% macro smi_reduce(program, op) -%}
#include "smi/reduce_operations.h"

__kernel void smi_kernel_reduce_{{ op.logical_port }}(char num_rank)
{

    __constant int SHIFT_REG = {{ op.shift_reg() }};

    SMI_Network_message mess;
    SMI_Network_message reduce;
    bool init = true;
    char sender_id = 0;
    const char credits_flow_control = 16; // choose it in order to have II=1
    // reduced results, organized in shift register to mask latency (of the design, not related to the particular operation used)
    {{ op.data_type }} __attribute__((register)) reduce_result[credits_flow_control][SHIFT_REG + 1];
    char data_recvd[credits_flow_control];
    bool send_credits = false; // true if (the root) has to send reduce request
    char credits = credits_flow_control; // the number of credits that I have
    char send_to = 0;
    char add_to[MAX_RANKS];   // for each rank tells to what element in the buffer we should add the received item
{% set reduce_send = program.create_group("reduce_send") %}
{% set reduce_recv = program.create_group("reduce_recv") %}
{% set ckr_data = program.create_group("ckr_data") %}
{% set cks_control = program.create_group("cks_control") %}

    for (int i = 0;i < credits_flow_control; i++)
    {
        data_recvd[i] = 0;
        #pragma unroll
        for(int j = 0; j < SHIFT_REG + 1; j++)
        {
            reduce_result[i][j] = {{ op.shift_reg_init() }};
        }
    }

    for (int i = 0; i < MAX_RANKS; i++)
    {
        add_to[i] = 0;
    }
    char current_buffer_element = 0;
    char add_to_root = 0;
    char contiguos_reads = 0;

    while (true)
    {
        bool valid = false;
        if (!send_credits)
        {
            switch (sender_id)
            {   // for the root, I have to receive from both sides
                case 0:
                    mess = read_channel_nb_intel({{ utils.channel_array("reduce_send") }}[{{ reduce_send.get_hw_port(op.logical_port) }}], &valid);
                    break;
                case 1: // read from CK_R, can be done by the root and by the non-root
                    mess = read_channel_nb_intel({{ utils.channel_array("ckr_data") }}[{{ ckr_data.get_hw_port(op.logical_port) }}], &valid);
                    break;
            }
            if (valid)
            {
                char a;
                if (sender_id == 0)
                {
                    // received root contribution to the reduced result
                    // apply reduce
                    char* ptr = mess.data;
                    {{ op.data_type }} data= *({{ op.data_type }}*) (ptr);
                    reduce_result[add_to_root][SHIFT_REG] = {{ op.reduce_op() }}(data, reduce_result[add_to_root][0]); //apply reduce
                    #pragma unroll
                    for (int j = 0; j < SHIFT_REG; j++)
                    {
                        reduce_result[add_to_root][j] = reduce_result[add_to_root][j + 1];
                    }

                    data_recvd[add_to_root]++;
                    a = add_to_root;
                    send_credits = init;      // the first reduce, we send this
                    init = false;
                    add_to_root++;
                    if (add_to_root == credits_flow_control)
                    {
                        add_to_root = 0;
                    }
                }
                else
                {
                    // received contribution from a non-root rank, apply reduce operation
                    contiguos_reads++;
                    char* ptr = mess.data;
                    char rank = GET_HEADER_SRC(mess.header);
                    {{ op.data_type }} data = *({{ op.data_type }}*)(ptr);
                    char addto = add_to[rank];
                    data_recvd[addto]++;
                    a = addto;
                    reduce_result[addto][SHIFT_REG] = {{ op.reduce_op() }}(data,reduce_result[addto][0]);        // apply reduce
                    #pragma unroll
                    for (int j = 0; j < SHIFT_REG; j++)
                    {
                        reduce_result[addto][j] = reduce_result[addto][j + 1];
                    }

                    addto++;
                    if (addto == credits_flow_control)
                    {
                        addto = 0;
                    }
                    add_to[rank] = addto;
                }

                if (data_recvd[current_buffer_element] == num_rank)
                {
                    // We received all the contributions, we can send result to application
                    char* data_snd = reduce.data;
                    // Build reduced result
                    {{ op.data_type }} res = 0;
                    #pragma unroll
                    for (int i = 0; i < SHIFT_REG; i++)
                    {
                        res = {{ op.reduce_op() }}(res,reduce_result[current_buffer_element][i]);
                    }
                    char* conv = (char*)(&res);
                    #pragma unroll
                    for (int jj = 0; jj < {{ op.data_size() }}; jj++) // copy the data
                    {
                        data_snd[jj] = conv[jj];
                    }
                    write_channel_intel({{ utils.channel_array("reduce_recv") }}[{{ reduce_recv.get_hw_port(op.logical_port)}}], reduce);
                    send_credits = true;
                    credits++;
                    data_recvd[current_buffer_element] = 0;

                    //reset shift register
                    #pragma unroll
                    for (int j = 0; j < SHIFT_REG + 1; j++)
                    {
                        reduce_result[current_buffer_element][j] =  {{ op.shift_reg_init() }};
                    }
                    current_buffer_element++;
                    if (current_buffer_element == credits_flow_control)
                    {
                        current_buffer_element = 0;
                    }
                }
            }
            if (sender_id == 0)
            {
                sender_id = 1;
            }
            else if (!valid || contiguos_reads == READS_LIMIT)
            {
                sender_id = 0;
                contiguos_reads = 0;
            }
        }
        else
        {
            // send credits
            if (send_to != GET_HEADER_DST(mess.header))
            {
                SET_HEADER_OP(reduce.header, SMI_SYNCH);
                SET_HEADER_PORT(reduce.header, {{ op.logical_port }});
                SET_HEADER_DST(reduce.header,send_to);
                write_channel_intel({{ utils.channel_array("cks_control") }}[{{ cks_control.get_hw_port(op.logical_port) }}], reduce);
            }
            send_to++;
            if (send_to == num_rank)
            {
                send_to = 0;
                credits--;
                send_credits = credits != 0;
            }
        }
    }
}
{%- endmacro %}

#define BUFFER_SIZE 

#include "smi/network_message.h"


// the maximum number of consecutive reads that each CKs/CKr can do from the same channel
#define READS_LIMIT 8
// maximum number of ranks in the cluster
#define MAX_RANKS 8
//P2P communications use synchronization
#define P2P_RENDEZVOUS

// QSFP channels
#ifndef SMI_EMULATION_RANK
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("kernel_output_ch0")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("kernel_input_ch0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("kernel_output_ch1")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("kernel_input_ch1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("kernel_output_ch2")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("kernel_input_ch2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("kernel_output_ch3")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("kernel_input_ch3")));
#else
#if SMI_EMULATION_RANK == 0
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c0_r1c0")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c0_r0c0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c1_r2c0")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c0_r0c1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c2_unconnected")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_unconnected_r0c2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c3_unconnected")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_unconnected_r0c3")));
#endif
#if SMI_EMULATION_RANK == 1
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c0_r0c0")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c0_r1c0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c1_r2c1")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c1_r1c1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c2_r3c1")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c1_r1c2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c3_unconnected")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_unconnected_r1c3")));
#endif
#if SMI_EMULATION_RANK == 2
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c0_r0c1")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c1_r2c0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c1_r1c1")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c1_r2c1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c2_unconnected")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_unconnected_r2c2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c3_unconnected")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_unconnected_r2c3")));
#endif
#if SMI_EMULATION_RANK == 3
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c0_unconnected")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_unconnected_r3c0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c1_r1c2")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c2_r3c1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c2_unconnected")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_unconnected_r3c2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c3_unconnected")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_unconnected_r3c3")));
#endif
#endif

// Push(0)
channel SMI_Network_message push_0_ckr_control __attribute__((depth(8)));
channel SMI_Network_message push_0_cks_data __attribute__((depth(16)));
// Pop(0)
channel SMI_Network_message pop_0_ckr_data __attribute__((depth(16)));
channel SMI_Network_message pop_0_cks_control __attribute__((depth(16)));
// Push(1)
channel SMI_Network_message push_1_ckr_control __attribute__((depth(16)));
channel SMI_Network_message push_1_cks_data __attribute__((depth(16)));
// Pop(2)
channel SMI_Network_message pop_2_ckr_data __attribute__((depth(8)));
channel SMI_Network_message pop_2_cks_control __attribute__((depth(16)));
// Broadcast(3)
channel SMI_Network_message broadcast_3_broadcast __attribute__((depth(1)));
channel SMI_Network_message broadcast_3_ckr_control __attribute__((depth(64)));
channel SMI_Network_message broadcast_3_ckr_data __attribute__((depth(64)));
channel SMI_Network_message broadcast_3_cks_control __attribute__((depth(16)));
channel SMI_Network_message broadcast_3_cks_data __attribute__((depth(16)));
// Broadcast(4)
channel SMI_Network_message broadcast_4_broadcast __attribute__((depth(1)));
channel SMI_Network_message broadcast_4_ckr_control __attribute__((depth(16)));
channel SMI_Network_message broadcast_4_ckr_data __attribute__((depth(16)));
channel SMI_Network_message broadcast_4_cks_control __attribute__((depth(16)));
channel SMI_Network_message broadcast_4_cks_data __attribute__((depth(16)));
// Push(5)
channel SMI_Network_message push_5_ckr_control __attribute__((depth(32)));
channel SMI_Network_message push_5_cks_data __attribute__((depth(16)));
// Broadcast(6)
channel SMI_Network_message reduce_6_ckr_control __attribute__((depth(16)));
channel SMI_Network_message reduce_6_ckr_data __attribute__((depth(16)));
channel SMI_Network_message reduce_6_cks_control __attribute__((depth(16)));
channel SMI_Network_message reduce_6_cks_data __attribute__((depth(16)));
channel SMI_Network_message reduce_6_reduce_recv __attribute__((depth(1)));
channel SMI_Network_message reduce_6_reduce_send __attribute__((depth(1)));
// Scatter(7)
channel SMI_Network_message scatter_7_ckr_control __attribute__((depth(16)));
channel SMI_Network_message scatter_7_ckr_data __attribute__((depth(16)));
channel SMI_Network_message scatter_7_cks_control __attribute__((depth(16)));
channel SMI_Network_message scatter_7_cks_data __attribute__((depth(16)));
channel SMI_Network_message scatter_7_scatter __attribute__((depth(1)));
// Gather(8)
channel SMI_Network_message gather_8_ckr_control __attribute__((depth(16)));
channel SMI_Network_message gather_8_ckr_data __attribute__((depth(16)));
channel SMI_Network_message gather_8_cks_control __attribute__((depth(16)));
channel SMI_Network_message gather_8_cks_data __attribute__((depth(16)));
channel SMI_Network_message gather_8_gather __attribute__((depth(1)));

__constant char QSFP_COUNT = 4;

// connect all CK_S together
channel SMI_Network_message channels_interconnect_ck_s[QSFP_COUNT*(QSFP_COUNT-1)] __attribute__((depth(16)));

// connect all CK_R together
channel SMI_Network_message channels_interconnect_ck_r[QSFP_COUNT*(QSFP_COUNT-1)] __attribute__((depth(16)));

// connect corresponding CK_S/CK_R pairs
channel SMI_Network_message channels_interconnect_ck_s_to_ck_r[QSFP_COUNT] __attribute__((depth(16)));

// connect corresponding CK_R/CK_S pairs
channel SMI_Network_message channels_interconnect_ck_r_to_ck_s[QSFP_COUNT] __attribute__((depth(16)));

#include "smi/pop.h"
#include "smi/push.h"
#include "smi/bcast.h"
#include "smi/reduce.h"
#include "smi/scatter.h"
#include "smi/gather.h"
#include "smi/communicator.h"

__kernel void smi_kernel_cks_0(__global volatile char *restrict rt, const char num_ranks)
{
    char external_routing_table[MAX_RANKS];
    for (int i = 0; i < MAX_RANKS; i++)
    {
        if (i < num_ranks)
        {
            external_routing_table[i] = rt[i];
        }
    }

    // number of CK_S - 1 + CK_R + 0 CKS hardware ports
    const char num_sender = 4;
    char sender_id = 0;
    SMI_Network_message message;

    char contiguous_reads = 0;

    while (1)
    {
        bool valid = false;
        switch (sender_id)
        {
            case 0:
                // receive from CK_S_1
                message = read_channel_nb_intel(channels_interconnect_ck_s[0], &valid);
                break;
            case 1:
                // receive from CK_S_2
                message = read_channel_nb_intel(channels_interconnect_ck_s[1], &valid);
                break;
            case 2:
                // receive from CK_S_3
                message = read_channel_nb_intel(channels_interconnect_ck_s[2], &valid);
                break;
            case 3:
                // receive from CK_R_0
                message = read_channel_nb_intel(channels_interconnect_ck_r_to_ck_s[0], &valid);
                break;
            case 4:
                // receive from Push(0)
                message = read_channel_nb_intel(push_0_cks_data, &valid);
                break;
            case 5:
                // receive from Broadcast(3)
                message = read_channel_nb_intel(broadcast_3_cks_control, &valid);
                break;
            case 6:
                // receive from Push(5)
                message = read_channel_nb_intel(push_5_cks_data, &valid);
                break;
            case 7:
                // receive from Scatter(7)
                message = read_channel_nb_intel(scatter_7_cks_data, &valid);
                break;
        }

        if (valid)
        {
            contiguous_reads++;
            char idx = external_routing_table[GET_HEADER_DST(message.header)];
            switch (idx)
            {
                case 0:
                    // send to QSFP
                    write_channel_intel(io_out_0, message);
                    break;
                case 1:
                    // send to CK_R_0
                    write_channel_intel(channels_interconnect_ck_s_to_ck_r[0], message);
                    break;
                case 2:
                    // send to CK_S_1
                    write_channel_intel(channels_interconnect_ck_s[3], message);
                    break;
                case 3:
                    // send to CK_S_2
                    write_channel_intel(channels_interconnect_ck_s[6], message);
                    break;
                case 4:
                    // send to CK_S_3
                    write_channel_intel(channels_interconnect_ck_s[9], message);
                    break;
            }
        }
        if (!valid || contiguous_reads == READS_LIMIT)
        {
            contiguous_reads = 0;
            sender_id++;
            if (sender_id == num_sender)
            {
                sender_id = 0;
            }
        }
    }
}
__kernel void smi_kernel_ckr_0(__global volatile char *restrict rt, const char rank)
{
    // rt contains intertwined (dp0, cp0, dp1, cp1, ...)
    char external_routing_table[9 /* logical port count */][2];
    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            external_routing_table[i][j] = rt[i * 2 + j];
        }
    }

    // QSFP + number of CK_Rs - 1 + CK_S
    const char num_sender = 5;
    char sender_id = 0;
    SMI_Network_message message;

    char contiguous_reads = 0;
    while (1)
    {
        bool valid = false;
        switch (sender_id)
        {
            case 0:
                // QSFP
                message = read_channel_nb_intel(io_in_0, &valid);
                break;
            case 1:
                // receive from CK_R_1
                message = read_channel_nb_intel(channels_interconnect_ck_r[0], &valid);
                break;
            case 2:
                // receive from CK_R_2
                message = read_channel_nb_intel(channels_interconnect_ck_r[1], &valid);
                break;
            case 3:
                // receive from CK_R_3
                message = read_channel_nb_intel(channels_interconnect_ck_r[2], &valid);
                break;
            case 4:
                // receive from CK_S_0
                message = read_channel_nb_intel(channels_interconnect_ck_s_to_ck_r[0], &valid);
                break;
        }

        if (valid)
        {
            contiguous_reads++;
            char dest;
            if (GET_HEADER_DST(message.header) != rank)
            {
                dest = 0;
            }
            else dest = external_routing_table[GET_HEADER_PORT(message.header)][GET_HEADER_OP(message.header) == SMI_SYNCH];

            switch (dest)
            {
                case 0:
                    // send to CK_S_0
                    write_channel_intel(channels_interconnect_ck_r_to_ck_s[0], message);
                    break;
                case 1:
                    // send to CK_R_1
                    write_channel_intel(channels_interconnect_ck_r[3], message);
                    break;
                case 2:
                    // send to CK_R_2
                    write_channel_intel(channels_interconnect_ck_r[6], message);
                    break;
                case 3:
                    // send to CK_R_3
                    write_channel_intel(channels_interconnect_ck_r[9], message);
                    break;
                case 4:
                    // send to Push(0)
                    write_channel_intel(push_0_ckr_control, message);
                    break;
                case 5:
                    // send to Broadcast(3)
                    write_channel_intel(broadcast_3_ckr_control, message);
                    break;
                case 6:
                    // send to Push(5)
                    write_channel_intel(push_5_ckr_control, message);
                    break;
                case 7:
                    // send to Scatter(7)
                    write_channel_intel(scatter_7_ckr_data, message);
                    break;
            }
        }

        if (!valid || contiguous_reads == READS_LIMIT)
        {
            contiguous_reads = 0;
            sender_id++;
            if (sender_id == num_sender)
            {
                sender_id = 0;
            }
        }
    }
}
__kernel void smi_kernel_cks_1(__global volatile char *restrict rt, const char num_ranks)
{
    char external_routing_table[MAX_RANKS];
    for (int i = 0; i < MAX_RANKS; i++)
    {
        if (i < num_ranks)
        {
            external_routing_table[i] = rt[i];
        }
    }

    // number of CK_S - 1 + CK_R + 0 CKS hardware ports
    const char num_sender = 4;
    char sender_id = 0;
    SMI_Network_message message;

    char contiguous_reads = 0;

    while (1)
    {
        bool valid = false;
        switch (sender_id)
        {
            case 0:
                // receive from CK_S_0
                message = read_channel_nb_intel(channels_interconnect_ck_s[3], &valid);
                break;
            case 1:
                // receive from CK_S_2
                message = read_channel_nb_intel(channels_interconnect_ck_s[4], &valid);
                break;
            case 2:
                // receive from CK_S_3
                message = read_channel_nb_intel(channels_interconnect_ck_s[5], &valid);
                break;
            case 3:
                // receive from CK_R_1
                message = read_channel_nb_intel(channels_interconnect_ck_r_to_ck_s[1], &valid);
                break;
            case 4:
                // receive from Pop(0)
                message = read_channel_nb_intel(pop_0_cks_control, &valid);
                break;
            case 5:
                // receive from Broadcast(3)
                message = read_channel_nb_intel(broadcast_3_cks_data, &valid);
                break;
            case 6:
                // receive from Broadcast(6)
                message = read_channel_nb_intel(reduce_6_cks_control, &valid);
                break;
            case 7:
                // receive from Gather(8)
                message = read_channel_nb_intel(gather_8_cks_control, &valid);
                break;
        }

        if (valid)
        {
            contiguous_reads++;
            char idx = external_routing_table[GET_HEADER_DST(message.header)];
            switch (idx)
            {
                case 0:
                    // send to QSFP
                    write_channel_intel(io_out_1, message);
                    break;
                case 1:
                    // send to CK_R_1
                    write_channel_intel(channels_interconnect_ck_s_to_ck_r[1], message);
                    break;
                case 2:
                    // send to CK_S_0
                    write_channel_intel(channels_interconnect_ck_s[0], message);
                    break;
                case 3:
                    // send to CK_S_2
                    write_channel_intel(channels_interconnect_ck_s[7], message);
                    break;
                case 4:
                    // send to CK_S_3
                    write_channel_intel(channels_interconnect_ck_s[10], message);
                    break;
            }
        }
        if (!valid || contiguous_reads == READS_LIMIT)
        {
            contiguous_reads = 0;
            sender_id++;
            if (sender_id == num_sender)
            {
                sender_id = 0;
            }
        }
    }
}
__kernel void smi_kernel_ckr_1(__global volatile char *restrict rt, const char rank)
{
    // rt contains intertwined (dp0, cp0, dp1, cp1, ...)
    char external_routing_table[9 /* logical port count */][2];
    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            external_routing_table[i][j] = rt[i * 2 + j];
        }
    }

    // QSFP + number of CK_Rs - 1 + CK_S
    const char num_sender = 5;
    char sender_id = 0;
    SMI_Network_message message;

    char contiguous_reads = 0;
    while (1)
    {
        bool valid = false;
        switch (sender_id)
        {
            case 0:
                // QSFP
                message = read_channel_nb_intel(io_in_1, &valid);
                break;
            case 1:
                // receive from CK_R_0
                message = read_channel_nb_intel(channels_interconnect_ck_r[3], &valid);
                break;
            case 2:
                // receive from CK_R_2
                message = read_channel_nb_intel(channels_interconnect_ck_r[4], &valid);
                break;
            case 3:
                // receive from CK_R_3
                message = read_channel_nb_intel(channels_interconnect_ck_r[5], &valid);
                break;
            case 4:
                // receive from CK_S_1
                message = read_channel_nb_intel(channels_interconnect_ck_s_to_ck_r[1], &valid);
                break;
        }

        if (valid)
        {
            contiguous_reads++;
            char dest;
            if (GET_HEADER_DST(message.header) != rank)
            {
                dest = 0;
            }
            else dest = external_routing_table[GET_HEADER_PORT(message.header)][GET_HEADER_OP(message.header) == SMI_SYNCH];

            switch (dest)
            {
                case 0:
                    // send to CK_S_1
                    write_channel_intel(channels_interconnect_ck_r_to_ck_s[1], message);
                    break;
                case 1:
                    // send to CK_R_0
                    write_channel_intel(channels_interconnect_ck_r[0], message);
                    break;
                case 2:
                    // send to CK_R_2
                    write_channel_intel(channels_interconnect_ck_r[7], message);
                    break;
                case 3:
                    // send to CK_R_3
                    write_channel_intel(channels_interconnect_ck_r[10], message);
                    break;
                case 4:
                    // send to Pop(0)
                    write_channel_intel(pop_0_ckr_data, message);
                    break;
                case 5:
                    // send to Broadcast(3)
                    write_channel_intel(broadcast_3_ckr_data, message);
                    break;
                case 6:
                    // send to Broadcast(6)
                    write_channel_intel(reduce_6_ckr_control, message);
                    break;
                case 7:
                    // send to Gather(8)
                    write_channel_intel(gather_8_ckr_control, message);
                    break;
            }
        }

        if (!valid || contiguous_reads == READS_LIMIT)
        {
            contiguous_reads = 0;
            sender_id++;
            if (sender_id == num_sender)
            {
                sender_id = 0;
            }
        }
    }
}
__kernel void smi_kernel_cks_2(__global volatile char *restrict rt, const char num_ranks)
{
    char external_routing_table[MAX_RANKS];
    for (int i = 0; i < MAX_RANKS; i++)
    {
        if (i < num_ranks)
        {
            external_routing_table[i] = rt[i];
        }
    }

    // number of CK_S - 1 + CK_R + 0 CKS hardware ports
    const char num_sender = 4;
    char sender_id = 0;
    SMI_Network_message message;

    char contiguous_reads = 0;

    while (1)
    {
        bool valid = false;
        switch (sender_id)
        {
            case 0:
                // receive from CK_S_0
                message = read_channel_nb_intel(channels_interconnect_ck_s[6], &valid);
                break;
            case 1:
                // receive from CK_S_1
                message = read_channel_nb_intel(channels_interconnect_ck_s[7], &valid);
                break;
            case 2:
                // receive from CK_S_3
                message = read_channel_nb_intel(channels_interconnect_ck_s[8], &valid);
                break;
            case 3:
                // receive from CK_R_2
                message = read_channel_nb_intel(channels_interconnect_ck_r_to_ck_s[2], &valid);
                break;
            case 4:
                // receive from Push(1)
                message = read_channel_nb_intel(push_1_cks_data, &valid);
                break;
            case 5:
                // receive from Broadcast(4)
                message = read_channel_nb_intel(broadcast_4_cks_control, &valid);
                break;
            case 6:
                // receive from Broadcast(6)
                message = read_channel_nb_intel(reduce_6_cks_data, &valid);
                break;
            case 7:
                // receive from Gather(8)
                message = read_channel_nb_intel(gather_8_cks_data, &valid);
                break;
        }

        if (valid)
        {
            contiguous_reads++;
            char idx = external_routing_table[GET_HEADER_DST(message.header)];
            switch (idx)
            {
                case 0:
                    // send to QSFP
                    write_channel_intel(io_out_2, message);
                    break;
                case 1:
                    // send to CK_R_2
                    write_channel_intel(channels_interconnect_ck_s_to_ck_r[2], message);
                    break;
                case 2:
                    // send to CK_S_0
                    write_channel_intel(channels_interconnect_ck_s[1], message);
                    break;
                case 3:
                    // send to CK_S_1
                    write_channel_intel(channels_interconnect_ck_s[4], message);
                    break;
                case 4:
                    // send to CK_S_3
                    write_channel_intel(channels_interconnect_ck_s[11], message);
                    break;
            }
        }
        if (!valid || contiguous_reads == READS_LIMIT)
        {
            contiguous_reads = 0;
            sender_id++;
            if (sender_id == num_sender)
            {
                sender_id = 0;
            }
        }
    }
}
__kernel void smi_kernel_ckr_2(__global volatile char *restrict rt, const char rank)
{
    // rt contains intertwined (dp0, cp0, dp1, cp1, ...)
    char external_routing_table[9 /* logical port count */][2];
    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            external_routing_table[i][j] = rt[i * 2 + j];
        }
    }

    // QSFP + number of CK_Rs - 1 + CK_S
    const char num_sender = 5;
    char sender_id = 0;
    SMI_Network_message message;

    char contiguous_reads = 0;
    while (1)
    {
        bool valid = false;
        switch (sender_id)
        {
            case 0:
                // QSFP
                message = read_channel_nb_intel(io_in_2, &valid);
                break;
            case 1:
                // receive from CK_R_0
                message = read_channel_nb_intel(channels_interconnect_ck_r[6], &valid);
                break;
            case 2:
                // receive from CK_R_1
                message = read_channel_nb_intel(channels_interconnect_ck_r[7], &valid);
                break;
            case 3:
                // receive from CK_R_3
                message = read_channel_nb_intel(channels_interconnect_ck_r[8], &valid);
                break;
            case 4:
                // receive from CK_S_2
                message = read_channel_nb_intel(channels_interconnect_ck_s_to_ck_r[2], &valid);
                break;
        }

        if (valid)
        {
            contiguous_reads++;
            char dest;
            if (GET_HEADER_DST(message.header) != rank)
            {
                dest = 0;
            }
            else dest = external_routing_table[GET_HEADER_PORT(message.header)][GET_HEADER_OP(message.header) == SMI_SYNCH];

            switch (dest)
            {
                case 0:
                    // send to CK_S_2
                    write_channel_intel(channels_interconnect_ck_r_to_ck_s[2], message);
                    break;
                case 1:
                    // send to CK_R_0
                    write_channel_intel(channels_interconnect_ck_r[1], message);
                    break;
                case 2:
                    // send to CK_R_1
                    write_channel_intel(channels_interconnect_ck_r[4], message);
                    break;
                case 3:
                    // send to CK_R_3
                    write_channel_intel(channels_interconnect_ck_r[11], message);
                    break;
                case 4:
                    // send to Push(1)
                    write_channel_intel(push_1_ckr_control, message);
                    break;
                case 5:
                    // send to Broadcast(4)
                    write_channel_intel(broadcast_4_ckr_control, message);
                    break;
                case 6:
                    // send to Broadcast(6)
                    write_channel_intel(reduce_6_ckr_data, message);
                    break;
                case 7:
                    // send to Gather(8)
                    write_channel_intel(gather_8_ckr_data, message);
                    break;
            }
        }

        if (!valid || contiguous_reads == READS_LIMIT)
        {
            contiguous_reads = 0;
            sender_id++;
            if (sender_id == num_sender)
            {
                sender_id = 0;
            }
        }
    }
}
__kernel void smi_kernel_cks_3(__global volatile char *restrict rt, const char num_ranks)
{
    char external_routing_table[MAX_RANKS];
    for (int i = 0; i < MAX_RANKS; i++)
    {
        if (i < num_ranks)
        {
            external_routing_table[i] = rt[i];
        }
    }

    // number of CK_S - 1 + CK_R + 0 CKS hardware ports
    const char num_sender = 4;
    char sender_id = 0;
    SMI_Network_message message;

    char contiguous_reads = 0;

    while (1)
    {
        bool valid = false;
        switch (sender_id)
        {
            case 0:
                // receive from CK_S_0
                message = read_channel_nb_intel(channels_interconnect_ck_s[9], &valid);
                break;
            case 1:
                // receive from CK_S_1
                message = read_channel_nb_intel(channels_interconnect_ck_s[10], &valid);
                break;
            case 2:
                // receive from CK_S_2
                message = read_channel_nb_intel(channels_interconnect_ck_s[11], &valid);
                break;
            case 3:
                // receive from CK_R_3
                message = read_channel_nb_intel(channels_interconnect_ck_r_to_ck_s[3], &valid);
                break;
            case 4:
                // receive from Pop(2)
                message = read_channel_nb_intel(pop_2_cks_control, &valid);
                break;
            case 5:
                // receive from Broadcast(4)
                message = read_channel_nb_intel(broadcast_4_cks_data, &valid);
                break;
            case 6:
                // receive from Scatter(7)
                message = read_channel_nb_intel(scatter_7_cks_control, &valid);
                break;
        }

        if (valid)
        {
            contiguous_reads++;
            char idx = external_routing_table[GET_HEADER_DST(message.header)];
            switch (idx)
            {
                case 0:
                    // send to QSFP
                    write_channel_intel(io_out_3, message);
                    break;
                case 1:
                    // send to CK_R_3
                    write_channel_intel(channels_interconnect_ck_s_to_ck_r[3], message);
                    break;
                case 2:
                    // send to CK_S_0
                    write_channel_intel(channels_interconnect_ck_s[2], message);
                    break;
                case 3:
                    // send to CK_S_1
                    write_channel_intel(channels_interconnect_ck_s[5], message);
                    break;
                case 4:
                    // send to CK_S_2
                    write_channel_intel(channels_interconnect_ck_s[8], message);
                    break;
            }
        }
        if (!valid || contiguous_reads == READS_LIMIT)
        {
            contiguous_reads = 0;
            sender_id++;
            if (sender_id == num_sender)
            {
                sender_id = 0;
            }
        }
    }
}
__kernel void smi_kernel_ckr_3(__global volatile char *restrict rt, const char rank)
{
    // rt contains intertwined (dp0, cp0, dp1, cp1, ...)
    char external_routing_table[9 /* logical port count */][2];
    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            external_routing_table[i][j] = rt[i * 2 + j];
        }
    }

    // QSFP + number of CK_Rs - 1 + CK_S
    const char num_sender = 5;
    char sender_id = 0;
    SMI_Network_message message;

    char contiguous_reads = 0;
    while (1)
    {
        bool valid = false;
        switch (sender_id)
        {
            case 0:
                // QSFP
                message = read_channel_nb_intel(io_in_3, &valid);
                break;
            case 1:
                // receive from CK_R_0
                message = read_channel_nb_intel(channels_interconnect_ck_r[9], &valid);
                break;
            case 2:
                // receive from CK_R_1
                message = read_channel_nb_intel(channels_interconnect_ck_r[10], &valid);
                break;
            case 3:
                // receive from CK_R_2
                message = read_channel_nb_intel(channels_interconnect_ck_r[11], &valid);
                break;
            case 4:
                // receive from CK_S_3
                message = read_channel_nb_intel(channels_interconnect_ck_s_to_ck_r[3], &valid);
                break;
        }

        if (valid)
        {
            contiguous_reads++;
            char dest;
            if (GET_HEADER_DST(message.header) != rank)
            {
                dest = 0;
            }
            else dest = external_routing_table[GET_HEADER_PORT(message.header)][GET_HEADER_OP(message.header) == SMI_SYNCH];

            switch (dest)
            {
                case 0:
                    // send to CK_S_3
                    write_channel_intel(channels_interconnect_ck_r_to_ck_s[3], message);
                    break;
                case 1:
                    // send to CK_R_0
                    write_channel_intel(channels_interconnect_ck_r[2], message);
                    break;
                case 2:
                    // send to CK_R_1
                    write_channel_intel(channels_interconnect_ck_r[5], message);
                    break;
                case 3:
                    // send to CK_R_2
                    write_channel_intel(channels_interconnect_ck_r[8], message);
                    break;
                case 4:
                    // send to Pop(2)
                    write_channel_intel(pop_2_ckr_data, message);
                    break;
                case 5:
                    // send to Broadcast(4)
                    write_channel_intel(broadcast_4_ckr_data, message);
                    break;
                case 6:
                    // send to Scatter(7)
                    write_channel_intel(scatter_7_ckr_control, message);
                    break;
            }
        }

        if (!valid || contiguous_reads == READS_LIMIT)
        {
            contiguous_reads = 0;
            sender_id++;
            if (sender_id == num_sender)
            {
                sender_id = 0;
            }
        }
    }
}

// Push
SMI_Channel SMI_Open_send_channel_0_short(int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm)
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
    chan.elements_per_packet = 14;
    chan.max_tokens = 112;

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
SMI_Channel SMI_Open_send_channel_1_int(int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm)
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
    chan.elements_per_packet = 7;
    chan.max_tokens = 112;

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
SMI_Channel SMI_Open_send_channel_5_double(int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm)
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
    chan.elements_per_packet = 3;
    chan.max_tokens = 96;

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

void SMI_Push_flush_0_short(SMI_Channel *chan, void* data, int immediate)
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
        write_channel_intel(push_0_cks_data, chan->net);
    }
    // This fence is not mandatory, the two channel operations can be
    // performed independently
    // mem_fence(CLK_CHANNEL_MEM_FENCE);

    if (chan->tokens == 0)
    {
        // receives also with tokens=0
        // wait until the message arrives
        SMI_Network_message mess = read_channel_intel(push_0_ckr_control);
        unsigned int tokens = *(unsigned int *) mess.data;
        chan->tokens += tokens; // tokens
    }
}
void SMI_Push_0_short(SMI_Channel *chan, void* data)
{
    SMI_Push_flush_0_short(chan, data, 0);
}
void SMI_Push_flush_1_int(SMI_Channel *chan, void* data, int immediate)
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
        write_channel_intel(push_1_cks_data, chan->net);
    }
    // This fence is not mandatory, the two channel operations can be
    // performed independently
    // mem_fence(CLK_CHANNEL_MEM_FENCE);

    if (chan->tokens == 0)
    {
        // receives also with tokens=0
        // wait until the message arrives
        SMI_Network_message mess = read_channel_intel(push_1_ckr_control);
        unsigned int tokens = *(unsigned int *) mess.data;
        chan->tokens += tokens; // tokens
    }
}
void SMI_Push_1_int(SMI_Channel *chan, void* data)
{
    SMI_Push_flush_1_int(chan, data, 0);
}
void SMI_Push_flush_5_double(SMI_Channel *chan, void* data, int immediate)
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
        write_channel_intel(push_5_cks_data, chan->net);
    }
    // This fence is not mandatory, the two channel operations can be
    // performed independently
    // mem_fence(CLK_CHANNEL_MEM_FENCE);

    if (chan->tokens == 0)
    {
        // receives also with tokens=0
        // wait until the message arrives
        SMI_Network_message mess = read_channel_intel(push_5_ckr_control);
        unsigned int tokens = *(unsigned int *) mess.data;
        chan->tokens += tokens; // tokens
    }
}
void SMI_Push_5_double(SMI_Channel *chan, void* data)
{
    SMI_Push_flush_5_double(chan, data, 0);
}

// Pop
SMI_Channel SMI_Open_receive_channel_0_int(int count, SMI_Datatype data_type, int source, int port, SMI_Comm comm)
{
    SMI_Channel chan;
    // setup channel descriptor
    chan.port = (char) port;
    chan.sender_rank = (char) source;
    chan.message_size = (unsigned int) count;
    chan.data_type = data_type;
    chan.op_type = SMI_RECEIVE;
    chan.elements_per_packet = 7;
    chan.max_tokens = 112;

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
SMI_Channel SMI_Open_receive_channel_2_char(int count, SMI_Datatype data_type, int source, int port, SMI_Comm comm)
{
    SMI_Channel chan;
    // setup channel descriptor
    chan.port = (char) port;
    chan.sender_rank = (char) source;
    chan.message_size = (unsigned int) count;
    chan.data_type = data_type;
    chan.op_type = SMI_RECEIVE;
    chan.elements_per_packet = 28;
    chan.max_tokens = 224;

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

void SMI_Pop_0_int(SMI_Channel *chan, void *data)
{
    // in this case we have to copy the data into the target variable
    if (chan->packet_element_id == 0)
    {
        // no data to be unpacked...receive from the network
        chan->net = read_channel_intel(pop_0_ckr_data);
    }
    chan->processed_elements++;
    char *data_recvd = chan->net.data;

    #pragma unroll
    for (int ee = 0; ee < 7; ee++)
    {
        if (ee == chan->packet_element_id)
        {
            #pragma unroll
            for (int jj = 0; jj < 4; jj++)
            {
                ((char *)data)[jj] = data_recvd[(ee * 4) + jj];
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
        write_channel_intel(pop_0_cks_control, mess);
    }
}
void SMI_Pop_2_char(SMI_Channel *chan, void *data)
{
    // in this case we have to copy the data into the target variable
    if (chan->packet_element_id == 0)
    {
        // no data to be unpacked...receive from the network
        chan->net = read_channel_intel(pop_2_ckr_data);
    }
    chan->processed_elements++;
    char *data_recvd = chan->net.data;

    #pragma unroll
    for (int ee = 0; ee < 28; ee++)
    {
        if (ee == chan->packet_element_id)
        {
            #pragma unroll
            for (int jj = 0; jj < 1; jj++)
            {
                ((char *)data)[jj] = data_recvd[(ee * 1) + jj];
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
        write_channel_intel(pop_2_cks_control, mess);
    }
}

// Broadcast
__kernel void smi_kernel_bcast_3(char num_rank)
{
    bool external = true;
    char rcv;
    char root;
    char received_request = 0; // how many ranks are ready to receive
    const char num_requests = num_rank - 1;
    SMI_Network_message mess;

    while (true)
    {
        if (external) // read from the application
        {
            mess = read_channel_intel(broadcast_3_broadcast);
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
                SMI_Network_message req = read_channel_intel(broadcast_3_ckr_control);
                received_request--;
            }
            else
            {
                if (rcv != root) // it's not me
                {
                    SET_HEADER_DST(mess.header, rcv);
                    SET_HEADER_PORT(mess.header, 3);
                    write_channel_intel(broadcast_3_cks_data, mess);
                }
                rcv++;
                external = rcv == num_rank;
            }
        }
    }
}
__kernel void smi_kernel_bcast_4(char num_rank)
{
    bool external = true;
    char rcv;
    char root;
    char received_request = 0; // how many ranks are ready to receive
    const char num_requests = num_rank - 1;
    SMI_Network_message mess;

    while (true)
    {
        if (external) // read from the application
        {
            mess = read_channel_intel(broadcast_4_broadcast);
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
                SMI_Network_message req = read_channel_intel(broadcast_4_ckr_control);
                received_request--;
            }
            else
            {
                if (rcv != root) // it's not me
                {
                    SET_HEADER_DST(mess.header, rcv);
                    SET_HEADER_PORT(mess.header, 4);
                    write_channel_intel(broadcast_4_cks_data, mess);
                }
                rcv++;
                external = rcv == num_rank;
            }
        }
    }
}

SMI_BChannel SMI_Open_bcast_channel_3_float(int count, SMI_Datatype data_type, int port, int root, SMI_Comm comm)
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
    chan.size_of_type = 4;
    chan.elements_per_packet = 7;

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
SMI_BChannel SMI_Open_bcast_channel_4_int(int count, SMI_Datatype data_type, int port, int root, SMI_Comm comm)
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
    chan.size_of_type = 4;
    chan.elements_per_packet = 7;

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

void SMI_Bcast_3_float(SMI_BChannel* chan, void* data)
{
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
            SET_HEADER_PORT(chan->net.header, 3);
            chan->packet_element_id = 0;

            // offload to support kernel
            write_channel_intel(broadcast_3_broadcast, chan->net);
            SET_HEADER_OP(chan->net.header, SMI_BROADCAST);  // for the subsequent network packets
        }
    }
    else // I have to receive
    {
        if(chan->init)  //send ready-to-receive to the root
        {
            write_channel_intel(broadcast_3_cks_control, chan->net);
            chan->init=false;
        }

        if (chan->packet_element_id_rcv == 0)
        {
            chan->net_2 = read_channel_intel(broadcast_3_ckr_data);
        }

        COPY_DATA_FROM_NET_MESSAGE(chan, chan->net_2, conv);

        chan->packet_element_id_rcv++;
        if (chan->packet_element_id_rcv == chan->elements_per_packet)
        {
            chan->packet_element_id_rcv = 0;
        }
    }
}
void SMI_Bcast_4_int(SMI_BChannel* chan, void* data)
{
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
            SET_HEADER_PORT(chan->net.header, 4);
            chan->packet_element_id = 0;

            // offload to support kernel
            write_channel_intel(broadcast_4_broadcast, chan->net);
            SET_HEADER_OP(chan->net.header, SMI_BROADCAST);  // for the subsequent network packets
        }
    }
    else // I have to receive
    {
        if(chan->init)  //send ready-to-receive to the root
        {
            write_channel_intel(broadcast_4_cks_control, chan->net);
            chan->init=false;
        }

        if (chan->packet_element_id_rcv == 0)
        {
            chan->net_2 = read_channel_intel(broadcast_4_ckr_data);
        }

        COPY_DATA_FROM_NET_MESSAGE(chan, chan->net_2, conv);

        chan->packet_element_id_rcv++;
        if (chan->packet_element_id_rcv == chan->elements_per_packet)
        {
            chan->packet_element_id_rcv = 0;
        }
    }
}

// Scatter
__kernel void smi_kernel_scatter_7(char num_rank)
{
    bool external = true;
    char to_be_received_requests = 0; // how many ranks have still to communicate that they are ready to receive
    const char num_requests = num_rank - 1;
    SMI_Network_message mess;

    while (true)
    {
        if (external) // read from the application
        {
            mess = read_channel_intel(scatter_7_scatter);
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
                SMI_Network_message req = read_channel_intel(scatter_7_ckr_control);
                to_be_received_requests--;
            }
            else
            {
                // just push it to the network
                write_channel_intel(scatter_7_cks_data, mess);
                external = true;
            }
        }
    }
}

SMI_ScatterChannel SMI_Open_scatter_channel_7_double(int send_count, int recv_count,
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
    chan.size_of_type = 8;
    chan.elements_per_packet = 3;

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

void SMI_Scatter_7_double(SMI_ScatterChannel* chan, void* data_snd, void* data_rcv)
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
        for (int ee = 0; ee < 3; ee++)
        {
            if (ee == chan->packet_element_id)
            {
                #pragma unroll
                for (int jj = 0; jj < 8; jj++)
                {
                    if (chan->next_rcv == chan->my_rank)
                    {
                        ((char*) (data_rcv))[jj] = conv[jj];
                    }
                    else data_to_send[(ee * 8) + jj] = conv[jj];
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
                write_channel_intel(scatter_7_scatter, chan->net);
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
            write_channel_intel(scatter_7_cks_control, chan->net);
            chan->init=false;
        }
        if (chan->packet_element_id_rcv == 0)
        {
            chan->net_2 = read_channel_intel(scatter_7_ckr_data);
        }
        COPY_DATA_FROM_NET_MESSAGE(chan, chan->net_2, data_rcv);

        chan->packet_element_id_rcv++;
        if (chan->packet_element_id_rcv == elem_per_packet)
        {
            chan->packet_element_id_rcv = 0;
        }
    }
}

// Gather
__kernel void smi_kernel_gather_8(char num_rank)
{
    // receives the data from the application and
    // forwards it to the root only when the SYNCH message arrives
    SMI_Network_message mess;
    SMI_Network_message req;

    while (true)
    {
        mess = read_channel_intel(gather_8_gather);
        if (GET_HEADER_OP(mess.header) == SMI_SYNCH)
        {
            req = read_channel_intel(gather_8_ckr_control);
        }

        // we introduce a dependency on the received synchronization message to
        // force the ordering of the two channel operations

        SET_HEADER_NUM_ELEMS(mess.header, MAX(GET_HEADER_NUM_ELEMS(mess.header), GET_HEADER_NUM_ELEMS(req.header)));

        SET_HEADER_OP(mess.header, SMI_GATHER);
        write_channel_intel(gather_8_cks_data, mess);
    }
}

SMI_GatherChannel SMI_Open_gather_channel_8_char(int send_count, int recv_count, SMI_Datatype data_type, int port, int root, SMI_Comm comm)
{
    SMI_GatherChannel chan;
    chan.port = (char) port;
    chan.send_count = send_count;
    chan.recv_count = recv_count;
    chan.data_type = data_type;
    chan.my_rank = (char) SMI_Comm_rank(comm);
    chan.root_rank = (char) root;
    chan.num_rank = (char) SMI_Comm_size(comm);
    chan.next_contrib = 0;
    chan.size_of_type = 1;
    chan.elements_per_packet = 28;

    // setup header for the message
    SET_HEADER_SRC(chan.net.header, chan.my_rank);
    SET_HEADER_PORT(chan.net.header, chan.port);
    SET_HEADER_NUM_ELEMS(chan.net.header, 0);
    SET_HEADER_OP(chan.net.header, SMI_SYNCH);
    // net_2 is used by the non-root ranks
    SET_HEADER_OP(chan.net_2.header, SMI_SYNCH);
    SET_HEADER_PORT(chan.net_2.header, chan.port);
    SET_HEADER_DST(chan.net_2.header, chan.root_rank);
    chan.processed_elements = 0;
    chan.processed_elements_root = 0;
    chan.packet_element_id = 0;
    chan.packet_element_id_rcv = 0;
    return chan;
}

/**
 * @brief SMI_Gather
 * @param chan pointer to the gather channel descriptor
 * @param data_snd pointer to the data element that must be sent
 * @param data_rcv pointer to the receiving data element (significant on the root rank only)
 */
void SMI_Gather_8_char(SMI_GatherChannel* chan, void* send_data, void* rcv_data)
{
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
            SET_HEADER_PORT(chan->net_2.header, 8);
            write_channel_intel(gather_8_cks_control, chan->net_2);
        }
        // This fence is not necessary, the two channel operation are independent
        // and we don't need any ordering between them
        // mem_fence(CLK_CHANNEL_MEM_FENCE);

        // receive the data
        if (chan->packet_element_id_rcv == 0 && chan->next_contrib != chan->my_rank)
        {
            chan->net = read_channel_intel(gather_8_ckr_data);
        }

        char* data_recvd = chan->net.data;
        char* conv = (char*) send_data;

        #pragma unroll
        for (int ee = 0; ee < 28; ee++)
        {
            if (ee == chan->packet_element_id_rcv)
            {
                #pragma unroll
                for (int jj = 0; jj < 1; jj++)
                {
                    if (chan->next_contrib != chan->root_rank)     // not my turn
                    {
                        ((char *)rcv_data)[jj] = data_recvd[(ee * 1) + jj];
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
        for (char jj = 0; jj < 1; jj++)
        {
            data_snd[chan->packet_element_id * 1 + jj] = conv[jj];
        }
        chan->packet_element_id++;

        if (chan->packet_element_id == chan->elements_per_packet || chan->processed_elements == message_size)
        {
            SET_HEADER_NUM_ELEMS(chan->net.header, chan->packet_element_id);
            SET_HEADER_DST(chan->net.header, chan->root_rank);

            write_channel_intel(gather_8_gather, chan->net);
            // first one is a SMI_SYNCH, used to communicate to the support kernel that it has to wait for a "ready to receive" from the root
            SET_HEADER_OP(chan->net.header, SMI_GATHER);
            chan->packet_element_id = 0;
        }
    }
}

// Reduce
#include "smi/reduce_operations.h"

__kernel void smi_kernel_reduce_6(char num_rank)
{
    __constant int SHIFT_REG = 4;

    SMI_Network_message mess;
    SMI_Network_message reduce;
    char sender_id = 0;
    const char credits_flow_control = 16; // choose it in order to have II=1
    // reduced results, organized in shift register to mask latency (of the design, not related to the particular operation used)
    float __attribute__((register)) reduce_result[credits_flow_control][SHIFT_REG + 1];
    char data_recvd[credits_flow_control];
    bool send_credits = false; // true if (the root) has to send reduce request
    char credits = credits_flow_control; // the number of credits that I have
    char send_to = 0;
    char add_to[MAX_RANKS];   // for each rank tells to what element in the buffer we should add the received item
    unsigned int sent_credits = 0;    //number of sent credits so far
    unsigned int message_size;

    for (int i = 0;i < credits_flow_control; i++)
    {
        data_recvd[i] = 0;
        #pragma unroll
        for(int j = 0; j < SHIFT_REG + 1; j++)
        {
            reduce_result[i][j] = 0;
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
                    mess = read_channel_nb_intel(reduce_6_reduce_send, &valid);
                    break;
                case 1: // read from CK_R, can be done by the root and by the non-root
                    mess = read_channel_nb_intel(reduce_6_ckr_data, &valid);
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
                    float data= *(float*) (ptr);
                    reduce_result[add_to_root][SHIFT_REG] = SMI_OP_ADD(data, reduce_result[add_to_root][0]); // apply reduce
                    #pragma unroll
                    for (int j = 0; j < SHIFT_REG; j++)
                    {
                        reduce_result[add_to_root][j] = reduce_result[add_to_root][j + 1];
                    }

                    data_recvd[add_to_root]++;
                    a = add_to_root;
                    if (GET_HEADER_OP(mess.header) == SMI_SYNCH) // first element of a new reduce
                    {
                        sent_credits = 0;
                        send_to = 0;
                        // since data elements are not packed we exploit the data buffer
                        // to indicate to the support kernel the lenght of the message
                        message_size = *(unsigned int *) (&(mess.data[24]));
                        send_credits = true;
                        credits = MIN((unsigned int) credits_flow_control, message_size);
                    }
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
                    float data = *(float*)(ptr);
                    char addto = add_to[rank];
                    data_recvd[addto]++;
                    a = addto;
                    reduce_result[addto][SHIFT_REG] = SMI_OP_ADD(data, reduce_result[addto][0]);        // apply reduce
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
                    float res = 0;
                    #pragma unroll
                    for (int i = 0; i < SHIFT_REG; i++)
                    {
                        res = SMI_OP_ADD(res,reduce_result[current_buffer_element][i]);
                    }
                    char* conv = (char*)(&res);
                    #pragma unroll
                    for (int jj = 0; jj < 4; jj++) // copy the data
                    {
                        data_snd[jj] = conv[jj];
                    }
                    write_channel_intel(reduce_6_reduce_recv, reduce);
                    send_credits = sent_credits<message_size;               // send additional tokes if there are other elements to reduce
                    credits++;
                    data_recvd[current_buffer_element] = 0;

                    //reset shift register
                    #pragma unroll
                    for (int j = 0; j < SHIFT_REG + 1; j++)
                    {
                        reduce_result[current_buffer_element][j] =  0;
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
                SET_HEADER_NUM_ELEMS(reduce.header,1);
                SET_HEADER_PORT(reduce.header, 6);
                SET_HEADER_DST(reduce.header, send_to);
                write_channel_intel(reduce_6_cks_control, reduce);
            }
            send_to++;
            if (send_to == num_rank)
            {
                send_to = 0;
                credits--;
                send_credits = credits != 0;
                sent_credits++;
            }
        }
    }
}

SMI_RChannel SMI_Open_reduce_channel_6_float(int count, SMI_Datatype data_type, SMI_Op op, int port, int root, SMI_Comm comm)
{
    SMI_RChannel chan;
    // setup channel descriptor
    chan.message_size = (unsigned int) count;
    chan.data_type = data_type;
    chan.port = (char) port;
    chan.my_rank = (char) SMI_Comm_rank(comm);
    chan.root_rank = (char) root;
    chan.num_rank = (char) SMI_Comm_size(comm);
    chan.reduce_op = (char) op;
    chan.size_of_type = 4;
    chan.elements_per_packet = 7;

    // setup header for the message
    SET_HEADER_DST(chan.net.header, chan.root_rank);
    SET_HEADER_SRC(chan.net.header, chan.my_rank);
    SET_HEADER_PORT(chan.net.header, chan.port);         // used by destination
    SET_HEADER_NUM_ELEMS(chan.net.header, 0);            // at the beginning no data
    // workaround: the support kernel has to know the message size to limit the number of credits
    // exploiting the data buffer
    *(unsigned int *)(&(chan.net.data[24])) = chan.message_size;
    SET_HEADER_OP(chan.net.header, SMI_SYNCH);
    chan.processed_elements = 0;
    chan.packet_element_id = 0;
    chan.packet_element_id_rcv = 0;
    return chan;
}

void SMI_Reduce_6_float(SMI_RChannel* chan,  void* data_snd, void* data_rcv)
{
    char* conv = (char*) data_snd;
    // copy data to the network message
    COPY_DATA_TO_NET_MESSAGE(chan, chan->net,conv);

    // In this case we disabled network packetization: so we can just send the data as soon as we have it
    SET_HEADER_NUM_ELEMS(chan->net.header, 1);

    if (chan->my_rank == chan->root_rank) // root
    {
        write_channel_intel(reduce_6_reduce_send, chan->net);
        SET_HEADER_OP(chan->net.header, SMI_REDUCE);          // after sending the first element of this reduce
        mem_fence(CLK_CHANNEL_MEM_FENCE);
        chan->net_2 = read_channel_intel(reduce_6_reduce_recv);
        // copy data from the network message to user variable
        COPY_DATA_FROM_NET_MESSAGE(chan, chan->net_2, data_rcv);
    }
    else
    {
        // wait for credits

        SMI_Network_message req = read_channel_intel(reduce_6_ckr_control);
        mem_fence(CLK_CHANNEL_MEM_FENCE);
        SET_HEADER_OP(chan->net.header,SMI_REDUCE);
        // then send the data
        write_channel_intel(reduce_6_cks_data, chan->net);
    }
}

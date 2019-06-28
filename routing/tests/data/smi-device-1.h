#define BUFFER_SIZE 4096

#include "smi/channel_helpers.h"

// the maximum number of consecutive reads that each CKs/CKr can do from the same channel
#define READS_LIMIT 8
// maximum number of ranks in the cluster
#define MAX_RANKS 8

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

/**
  These tables, defined at compile time, maps application endpoints (Port) to channels and are
  used by the compiler to lay down the circuitry. The data routing table is used by push (and collectives)
  to send the actual communication data, while the control is used by push (and collective) to receive
  control information (e.g. rendezvous data) from the pairs. There are also otehr channels for collective operations.
*/

// cks_data: logical port -> index in cks_data_table -> index in cks_data_channels
__constant char cks_data_table[7] = { 0, 1, -1, 2, 3, 4, 5 };
channel SMI_Network_message cks_data_channels[6] __attribute__((depth(16)));

// cks_control: logical port -> index in cks_control_table -> index in cks_control_channels
__constant char cks_control_table[7] = { 0, -1, 1, 2, 3, -1, 4 };
channel SMI_Network_message cks_control_channels[5] __attribute__((depth(16)));

// ckr_data: logical port -> index in ckr_data_table -> index in ckr_data_channels
__constant char ckr_data_table[7] = { 0, -1, 1, 2, 3, -1, 4 };
channel SMI_Network_message ckr_data_channels[5] __attribute__((depth(BUFFER_SIZE)));

// ckr_control: logical port -> index in ckr_control_table -> index in ckr_control_channels
__constant char ckr_control_table[7] = { 0, 1, -1, 2, 3, 4, 5 };
channel SMI_Network_message ckr_control_channels[6] __attribute__((depth(BUFFER_SIZE)));


// broadcast: logical port -> index in broadcast_table -> index in broadcast_channels
__constant char broadcast_table[7] = { -1, -1, -1, 0, 1, -1, -1 };
channel SMI_Network_message broadcast_channels[2] __attribute__((depth(2)));


// reduce_send: logical port -> index in reduce_send_table -> index in reduce_send_channels
__constant char reduce_send_table[7] = { -1, -1, -1, -1, -1, -1, 0 };
channel SMI_Network_message reduce_send_channels[1] __attribute__((depth(1)));

// reduce_recv: logical port -> index in reduce_recv_table -> index in reduce_recv_channels
__constant char reduce_recv_table[7] = { -1, -1, -1, -1, -1, -1, 0 };
channel SMI_Network_message reduce_recv_channels[1] __attribute__((depth(1)));


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

__kernel void smi_kernel_cks_0(__global volatile char *restrict rt, const int num_ranks)
{
    char external_routing_table[MAX_RANKS];
    for (int i = 0; i < MAX_RANKS; i++)
    {
        if (i < num_ranks)
        {
            external_routing_table[i] = rt[i];
        }
    }

    // number of CK_S - 1 + CK_R + 3 CKS hardware ports
    const char num_sender = 7;
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
                // receive from app channel with logical port 0, hardware port 0, method data
                message = read_channel_nb_intel(cks_data_channels[0], &valid);
                break;
            case 5:
                // receive from app channel with logical port 5, hardware port 4, method data
                message = read_channel_nb_intel(cks_data_channels[4], &valid);
                break;
            case 6:
                // receive from app channel with logical port 3, hardware port 2, method control
                message = read_channel_nb_intel(cks_control_channels[2], &valid);
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
    char external_routing_table[7 /* logical port count */][2];
    for (int i = 0; i < 7; i++)
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
                    // send to app channel with logical port 0, hardware port 0, method data
                    write_channel_intel(ckr_data_channels[0], message);
                    break;
                case 5:
                    // send to app channel with logical port 6, hardware port 4, method data
                    write_channel_intel(ckr_data_channels[4], message);
                    break;
                case 6:
                    // send to app channel with logical port 4, hardware port 3, method control
                    write_channel_intel(ckr_control_channels[3], message);
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
__kernel void smi_kernel_cks_1(__global volatile char *restrict rt, const int num_ranks)
{
    char external_routing_table[MAX_RANKS];
    for (int i = 0; i < MAX_RANKS; i++)
    {
        if (i < num_ranks)
        {
            external_routing_table[i] = rt[i];
        }
    }

    // number of CK_S - 1 + CK_R + 3 CKS hardware ports
    const char num_sender = 7;
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
                // receive from app channel with logical port 1, hardware port 1, method data
                message = read_channel_nb_intel(cks_data_channels[1], &valid);
                break;
            case 5:
                // receive from app channel with logical port 6, hardware port 5, method data
                message = read_channel_nb_intel(cks_data_channels[5], &valid);
                break;
            case 6:
                // receive from app channel with logical port 4, hardware port 3, method control
                message = read_channel_nb_intel(cks_control_channels[3], &valid);
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
    char external_routing_table[7 /* logical port count */][2];
    for (int i = 0; i < 7; i++)
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
                    // send to app channel with logical port 2, hardware port 1, method data
                    write_channel_intel(ckr_data_channels[1], message);
                    break;
                case 5:
                    // send to app channel with logical port 0, hardware port 0, method control
                    write_channel_intel(ckr_control_channels[0], message);
                    break;
                case 6:
                    // send to app channel with logical port 5, hardware port 4, method control
                    write_channel_intel(ckr_control_channels[4], message);
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
__kernel void smi_kernel_cks_2(__global volatile char *restrict rt, const int num_ranks)
{
    char external_routing_table[MAX_RANKS];
    for (int i = 0; i < MAX_RANKS; i++)
    {
        if (i < num_ranks)
        {
            external_routing_table[i] = rt[i];
        }
    }

    // number of CK_S - 1 + CK_R + 3 CKS hardware ports
    const char num_sender = 7;
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
                // receive from app channel with logical port 3, hardware port 2, method data
                message = read_channel_nb_intel(cks_data_channels[2], &valid);
                break;
            case 5:
                // receive from app channel with logical port 0, hardware port 0, method control
                message = read_channel_nb_intel(cks_control_channels[0], &valid);
                break;
            case 6:
                // receive from app channel with logical port 6, hardware port 4, method control
                message = read_channel_nb_intel(cks_control_channels[4], &valid);
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
    char external_routing_table[7 /* logical port count */][2];
    for (int i = 0; i < 7; i++)
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
                    // send to app channel with logical port 3, hardware port 2, method data
                    write_channel_intel(ckr_data_channels[2], message);
                    break;
                case 5:
                    // send to app channel with logical port 1, hardware port 1, method control
                    write_channel_intel(ckr_control_channels[1], message);
                    break;
                case 6:
                    // send to app channel with logical port 6, hardware port 5, method control
                    write_channel_intel(ckr_control_channels[5], message);
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
__kernel void smi_kernel_cks_3(__global volatile char *restrict rt, const int num_ranks)
{
    char external_routing_table[MAX_RANKS];
    for (int i = 0; i < MAX_RANKS; i++)
    {
        if (i < num_ranks)
        {
            external_routing_table[i] = rt[i];
        }
    }

    // number of CK_S - 1 + CK_R + 2 CKS hardware ports
    const char num_sender = 6;
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
                // receive from app channel with logical port 4, hardware port 3, method data
                message = read_channel_nb_intel(cks_data_channels[3], &valid);
                break;
            case 5:
                // receive from app channel with logical port 2, hardware port 1, method control
                message = read_channel_nb_intel(cks_control_channels[1], &valid);
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
    char external_routing_table[7 /* logical port count */][2];
    for (int i = 0; i < 7; i++)
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
                    // send to app channel with logical port 4, hardware port 3, method data
                    write_channel_intel(ckr_data_channels[3], message);
                    break;
                case 5:
                    // send to app channel with logical port 3, hardware port 2, method control
                    write_channel_intel(ckr_control_channels[2], message);
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
            mess = read_channel_intel(broadcast_channels[0]);
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
                SMI_Network_message req = read_channel_intel(ckr_control_channels[2]);
                received_request--;
            }
            else
            {
                if (rcv != root) // it's not me
                {
                    SET_HEADER_DST(mess.header, rcv);
                    SET_HEADER_PORT(mess.header, 3);
                    write_channel_intel(cks_data_channels[2], mess);
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
            mess = read_channel_intel(broadcast_channels[1]);
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
                SMI_Network_message req = read_channel_intel(ckr_control_channels[3]);
                received_request--;
            }
            else
            {
                if (rcv != root) // it's not me
                {
                    SET_HEADER_DST(mess.header, rcv);
                    SET_HEADER_PORT(mess.header, 4);
                    write_channel_intel(cks_data_channels[3], mess);
                }
                rcv++;
                external = rcv == num_rank;
            }
        }
    }
}

__kernel void smi_kernel_reduce_6(char num_rank)
{
    __constant int SHIFT_REG = 4;

    SMI_Network_message mess;
    SMI_Network_message reduce;
    bool init = true;
    char sender_id = 0;
    const char credits_flow_control = 16; // apparently, this combination (credits, max ranks) is the max that we can support with II=1
    // reduced results, organized in shift register to mask latency
    float __attribute__((register)) reduce_result[credits_flow_control][SHIFT_REG + 1];
    char data_recvd[credits_flow_control];
    bool send_credits = false; // true if (the root) has to send reduce request
    char credits = credits_flow_control; // the number of credits that I have
    char send_to = 0;
    char /*__attribute__((register))*/ add_to[MAX_RANKS];   // for each rank tells to what element in the buffer we should add the received item

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
                    mess = read_channel_nb_intel(reduce_send_channels[0], &valid);
                    break;
                case 1: // read from CK_R, can be done by the root and by the non-root
                    mess = read_channel_nb_intel(ckr_data_channels[4], &valid);
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
                    reduce_result[add_to_root][SHIFT_REG] = data + reduce_result[add_to_root][0];
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
                     data = *(float*)(ptr);
                    char addto = add_to[rank];
                    data_recvd[addto]++;
                    a = addto;
                    reduce_result[addto][SHIFT_REG] = data+reduce_result[addto][0];        // SMI_ADD
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
                        res += reduce_result[current_buffer_element][i];
                    }
                    char* conv = (char*)(&res);
                    #pragma unroll
                    for (int jj = 0; jj < 4; jj++) // copy the data
                    {
                        data_snd[jj] = conv[jj];
                    }
                    write_channel_intel(reduce_recv_channels[0], reduce);
                    send_credits = true;
                    credits++;
                    data_recvd[current_buffer_element] = 0;

                    #pragma unroll
                    for (int j = 0; j < SHIFT_REG + 1; j++)
                    {
                        reduce_result[current_buffer_element][j] = 0;
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
                SET_HEADER_PORT(reduce.header, 6);
                SET_HEADER_DST(reduce.header,send_to);
                write_channel_intel(cks_control_channels[4], reduce);
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

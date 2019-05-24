#include "smi/channel_helpers.h"


#define RANK_COUNT 8

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
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c0_r6c1")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r6c1_r0c0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c1_r2c0")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c0_r0c1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c2_r1c3")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c3_r0c2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c3_r1c2")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c2_r0c3")));
#endif
#if SMI_EMULATION_RANK == 1
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c0_r7c1")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r7c1_r1c0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c1_r3c0")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c0_r1c1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c2_r0c3")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c3_r1c2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c3_r0c2")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c2_r1c3")));
#endif
#if SMI_EMULATION_RANK == 2
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c0_r0c1")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c1_r2c0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c1_r4c0")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r4c0_r2c1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c2_r3c3")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c3_r2c2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c3_r3c2")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c2_r2c3")));
#endif
#if SMI_EMULATION_RANK == 3
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c0_r1c1")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c1_r3c0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c1_r5c0")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r5c0_r3c1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c2_r2c3")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c3_r3c2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c3_r2c2")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c2_r3c3")));
#endif
#if SMI_EMULATION_RANK == 4
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r4c0_r2c1")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r2c1_r4c0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r4c1_r6c0")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r6c0_r4c1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r4c2_r5c3")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r5c3_r4c2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r4c3_r5c2")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r5c2_r4c3")));
#endif
#if SMI_EMULATION_RANK == 5
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r5c0_r3c1")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r3c1_r5c0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r5c1_r7c0")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r7c0_r5c1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r5c2_r4c3")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r4c3_r5c2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r5c3_r4c2")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r4c2_r5c3")));
#endif
#if SMI_EMULATION_RANK == 6
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r6c0_r4c1")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r4c1_r6c0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r6c1_r0c0")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r0c0_r6c1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r6c2_r7c3")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r7c3_r6c2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r6c3_r7c2")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r7c2_r6c3")));
#endif
#if SMI_EMULATION_RANK == 7
channel SMI_Network_message io_out_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r7c0_r5c1")));
channel SMI_Network_message io_in_0 __attribute__((depth(16))) __attribute__((io("emulated_channel_r5c1_r7c0")));
channel SMI_Network_message io_out_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r7c1_r1c0")));
channel SMI_Network_message io_in_1 __attribute__((depth(16))) __attribute__((io("emulated_channel_r1c0_r7c1")));
channel SMI_Network_message io_out_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r7c2_r6c3")));
channel SMI_Network_message io_in_2 __attribute__((depth(16))) __attribute__((io("emulated_channel_r6c3_r7c2")));
channel SMI_Network_message io_out_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r7c3_r6c2")));
channel SMI_Network_message io_in_3 __attribute__((depth(16))) __attribute__((io("emulated_channel_r6c2_r7c3")));
#endif
#endif

// internal routing tables
__constant char internal_sender_rt[6] = { 0, 1, 2, 3, 4, 5 };
__constant char internal_receiver_rt[6] = { 0, 1, 2, 3, 4, 5 };

channel SMI_Network_message channels_to_ck_s[6] __attribute__((depth(16)));
channel SMI_Network_message channels_from_ck_r[4] __attribute__((depth(16)));

__constant char QSFP_COUNT = 4;

// connect all CK_S together
channel SMI_Network_message channels_interconnect_ck_s[QSFP_COUNT*(QSFP_COUNT-1)] __attribute__((depth(16)));

// connect all CK_R together
channel SMI_Network_message channels_interconnect_ck_r[QSFP_COUNT*(QSFP_COUNT-1)] __attribute__((depth(16)));

// connect corresponding CK_S/CK_R pairs
channel SMI_Network_message channels_interconnect_ck_s_to_ck_r[QSFP_COUNT] __attribute__((depth(16)));

// connect corresponding CK_R/CK_S pairs
channel SMI_Network_message channels_interconnect_ck_r_to_ck_s[QSFP_COUNT] __attribute__((depth(16)));

#include "bcast.h"
#include "reduce.h"

__kernel void CK_S_0(__global volatile char *restrict rt)
{
    char external_routing_table[RANK_COUNT];
    for (int i = 0; i < RANK_COUNT; i++)
    {
        external_routing_table[i] = rt[i];
    }

    // number of CK_S - 1 + CK_R + 2 tags
    const char num_sender = 6;
    char sender_id = 0;
    SMI_Network_message message;

    const char READS_LIMIT=8;
    char contiguos_reads=0;
    while(1)
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
                // receive from app channel with tag 0
                message = read_channel_nb_intel(channels_to_ck_s[0], &valid);
                break;
            case 5:
                // receive from app channel with tag 4
                message = read_channel_nb_intel(channels_to_ck_s[4], &valid);
                break;
        }

        if (valid)
        {
            contiguos_reads++;
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
__kernel void CK_R_0(__global volatile char *restrict rt, const char rank)
{
    char external_routing_table[6 /* tag count */];
    for (int i = 0; i < 6 /* tag count */; i++)
    {
        external_routing_table[i] = rt[i];
    }

    // QSFP + number of CK_Rs - 1 + CK_S
    const char num_sender = 5;
    char sender_id = 0;
    SMI_Network_message message;
    const char READS_LIMIT=8;
    char contiguos_reads=0;
    while(1)
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
                    // send to app channel with tag 0
                    write_channel_intel(channels_from_ck_r[internal_receiver_rt[0]], message);
                    break;
                /*case 5:
                    // send to app channel with tag 4
                    write_channel_intel(channels_from_ck_r[internal_receiver_rt[4]], message);
                    break;*/
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
__kernel void CK_S_1(__global volatile char *restrict rt)
{
    char external_routing_table[RANK_COUNT];
    for (int i = 0; i < RANK_COUNT; i++)
    {
        external_routing_table[i] = rt[i];
    }

    // number of CK_S - 1 + CK_R + 2 tags
    const char num_sender = 6;
    char sender_id = 0;
    SMI_Network_message message;

    const char READS_LIMIT=8;
    char contiguos_reads=0;
    while(1)
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
                // receive from app channel with tag 1
                message = read_channel_nb_intel(channels_to_ck_s[1], &valid);
                break;
            case 5:
                // receive from app channel with tag 5
                message = read_channel_nb_intel(channels_to_ck_s[5], &valid);
                break;
        }

        if (valid)
        {
            contiguos_reads++;
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
__kernel void CK_R_1(__global volatile char *restrict rt, const char rank)
{
    char external_routing_table[6 /* tag count */];
    for (int i = 0; i < 6 /* tag count */; i++)
    {
        external_routing_table[i] = rt[i];
    }

    // QSFP + number of CK_Rs - 1 + CK_S
    const char num_sender = 5;
    char sender_id = 0;
    SMI_Network_message message;
    const char READS_LIMIT=8;
    char contiguos_reads=0;
    while(1)
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
                    // send to app channel with tag 1
                    write_channel_intel(channels_from_ck_r[internal_receiver_rt[1]], message);
                    break;
               /* case 5:
                    // send to app channel with tag 5
                    write_channel_intel(channels_from_ck_r[internal_receiver_rt[5]], message);
                    break;*/
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
__kernel void CK_S_2(__global volatile char *restrict rt)
{
    char external_routing_table[RANK_COUNT];
    for (int i = 0; i < RANK_COUNT; i++)
    {
        external_routing_table[i] = rt[i];
    }

    // number of CK_S - 1 + CK_R + 1 tags
    const char num_sender = 5;
    char sender_id = 0;
    SMI_Network_message message;

    const char READS_LIMIT=8;
    char contiguos_reads=0;
    while(1)
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
                // receive from app channel with tag 2
                message = read_channel_nb_intel(channels_to_ck_s[2], &valid);
                break;
        }

        if (valid)
        {
            contiguos_reads++;
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
__kernel void CK_R_2(__global volatile char *restrict rt, const char rank)
{
    char external_routing_table[6 /* tag count */];
    for (int i = 0; i < 6 /* tag count */; i++)
    {
        external_routing_table[i] = rt[i];
    }

    // QSFP + number of CK_Rs - 1 + CK_S
    const char num_sender = 5;
    char sender_id = 0;
    SMI_Network_message message;
    const char READS_LIMIT=8;
    char contiguos_reads=0;
    while(1)
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
                    // send to app channel with tag 2
                    write_channel_intel(channels_from_ck_r[internal_receiver_rt[2]], message);
                    break;
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
__kernel void CK_S_3(__global volatile char *restrict rt)
{
    char external_routing_table[RANK_COUNT];
    for (int i = 0; i < RANK_COUNT; i++)
    {
        external_routing_table[i] = rt[i];
    }

    // number of CK_S - 1 + CK_R + 1 tags
    const char num_sender = 5;
    char sender_id = 0;
    SMI_Network_message message;

    const char READS_LIMIT=8;
    char contiguos_reads=0;
    while(1)
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
                // receive from app channel with tag 3
                message = read_channel_nb_intel(channels_to_ck_s[3], &valid);
                break;
        }

        if (valid)
        {
            contiguos_reads++;
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
__kernel void CK_R_3(__global volatile char *restrict rt, const char rank)
{
    char external_routing_table[6 /* tag count */];
    for (int i = 0; i < 6 /* tag count */; i++)
    {
        external_routing_table[i] = rt[i];
    }

    // QSFP + number of CK_Rs - 1 + CK_S
    const char num_sender = 5;
    char sender_id = 0;
    SMI_Network_message message;
    const char READS_LIMIT=8;
    char contiguos_reads=0;
    while(1)
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
                    // send to app channel with tag 3
                    write_channel_intel(channels_from_ck_r[internal_receiver_rt[3]], message);
                    break;
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

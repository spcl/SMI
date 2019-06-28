#define BUFFER_SIZE {{ program.buffer_size }}

#include "smi/channel_helpers.h"
{% import 'ckr.cl' as smi_ckr %}
{% import 'cks.cl' as smi_cks %}
{% import 'bcast.cl' as smi_bcast %}

// the maximum number of consecutive reads that each CKs/CKr can do from the same channel
#define READS_LIMIT 8
// maximum number of ranks in the cluster
#define MAX_RANKS 8

// QSFP channels
#ifndef SMI_EMULATION_RANK
{% for channel in channels %}
channel SMI_Network_message io_out_{{ channel.index }} __attribute__((depth(16))) __attribute__((io("kernel_output_ch{{ channel.index }}")));
channel SMI_Network_message io_in_{{ channel.index }} __attribute__((depth(16))) __attribute__((io("kernel_input_ch{{ channel.index }}")));
{% endfor %}
#else
{% for fpga in fpgas %}
#if SMI_EMULATION_RANK == {{ fpga.rank }}
    {% for channel in range(channels_per_fpga) %}
channel SMI_Network_message io_out_{{ channel }} __attribute__((depth(16))) __attribute__((io("emulated_channel_{{ channel_name(fpga.channels[channel], true) }}")));
channel SMI_Network_message io_in_{{ channel }} __attribute__((depth(16))) __attribute__((io("emulated_channel_{{ channel_name(fpga.channels[channel], false) }}")));
    {% endfor %}
#endif
{% endfor %}
#endif

/**
  These four tables, defined at compile time, maps application endpoints (Port) to CKs/CKr and are
  used by the compiler to lay down the circuitry. The data routing table is used by push (and collectives)
  to send the actual communication data, while the control is used by push (and collective) to receive
  control information (e.g. rendezvous data) from the pairs
*/
{% set cks_data = program.create_group(("cks", "data")) %}
{% set cks_control = program.create_group(("cks", "control")) %}
{% set ckr_data = program.create_group(("ckr", "data")) %}
{% set ckr_control = program.create_group(("ckr", "control")) %}
// logical port -> index in internal_cks/ckr -> index in channels_cks/ckr
__constant char internal_to_cks_data_rt[{{ program.logical_port_count }}] = { {{ cks_data.hw_mapping()|join(", ") }} };
__constant char internal_to_cks_control_rt[{{ program.logical_port_count }}] = { {{ cks_control.hw_mapping()|join(", ") }} };
__constant char internal_from_ckr_data_rt[{{ program.logical_port_count }}] = { {{ ckr_data.hw_mapping()|join(", ") }} };
__constant char internal_from_ckr_control_rt[{{ program.logical_port_count }}] = { {{ ckr_control.hw_mapping()|join(", ") }} };

channel SMI_Network_message channels_cks_data[{{ cks_data.hw_port_count }}] __attribute__((depth(16)));
channel SMI_Network_message channels_cks_control[{{ cks_control.hw_port_count }}] __attribute__((depth(16)));
channel SMI_Network_message channels_ckr_data[{{ ckr_data.hw_port_count }}] __attribute__((depth(BUFFER_SIZE)));
channel SMI_Network_message channels_ckr_control[{{ ckr_control.hw_port_count }}] __attribute__((depth(BUFFER_SIZE)));

// broadcast channels
{% set bcast = program.create_group("broadcast") %}
__constant char internal_bcast_rt[{{ program.logical_port_count }}] = { {{ bcast.hw_mapping()|join(", ")}} };
channel SMI_Network_message channels_bcast_send[{{ bcast.hw_port_count }}] __attribute__((depth(2)));

__constant char QSFP_COUNT = {{ channels_per_fpga }};

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

{% for channel in channels %}
{{ smi_cks.smi_cks(program, channel, channels|length, target_index) }}
{{ smi_ckr.smi_ckr(program, channel, channels|length, target_index) }}
{% endfor %}

{% for broadcast_op in program.get_collective_ops("broadcast") %}
{{ smi_bcast.smi_bcast(program, broadcast_op) }}
{% endfor %}

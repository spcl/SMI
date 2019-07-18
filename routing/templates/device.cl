#include "smi/network_message.h"
{% import 'utils.cl' as utils %}
{% import 'ckr.cl' as smi_ckr %}
{% import 'cks.cl' as smi_cks %}

{% import 'push.cl' as smi_push %}
{% import 'pop.cl' as smi_pop %}
{% import 'bcast.cl' as smi_bcast %}
{% import 'reduce.cl' as smi_reduce %}
{% import 'scatter.cl' as smi_scatter %}
{% import 'gather.cl' as smi_gather %}

// the maximum number of consecutive reads that each CKs/CKr can do from the same channel
#define READS_LIMIT {{ program.consecutive_read_limit }}
// maximum number of ranks in the cluster
#define MAX_RANKS {{ program.max_ranks }}
{% if program.p2p_rendezvous  %}
//P2P communications use synchronization
#define P2P_RENDEZVOUS
{% else %}
//P2P communications use eager transmission protocol
{% endif %}

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

{% for op in program.operations %}
// {{ op }}
{% for (channel, depth) in op.get_channel_defs() %}
channel SMI_Network_message {{ channel }} __attribute__((depth({{ depth }})));
{% endfor %}
{% endfor %}

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
#include "smi/reduce.h"
#include "smi/scatter.h"
#include "smi/gather.h"
#include "smi/communicator.h"

{% for channel in channels %}
{{ smi_cks.smi_cks(program, channel, channels|length, target_index) }}
{{ smi_ckr.smi_ckr(program, channel, channels|length, target_index) }}
{% endfor %}

{%- macro generate_op_impl(key, fn) %}
{% for op in program.get_ops_by_type(key) %}
{{ fn(program, op) }}
{% endfor %}
{%- endmacro %}

// Push
{{ generate_op_impl("push", smi_push.smi_push_channel) }}
{{ generate_op_impl("push", smi_push.smi_push_impl) }}
// Pop
{{ generate_op_impl("pop", smi_pop.smi_pop_channel) }}
{{ generate_op_impl("pop", smi_pop.smi_pop_impl) }}
// Broadcast
{{ generate_op_impl("broadcast", smi_bcast.smi_bcast_kernel) }}
{{ generate_op_impl("broadcast", smi_bcast.smi_bcast_channel) }}
{{ generate_op_impl("broadcast", smi_bcast.smi_bcast_impl) }}
// Scatter
{{ generate_op_impl("scatter", smi_scatter.smi_scatter_kernel) }}
{{ generate_op_impl("scatter", smi_scatter.smi_scatter_channel) }}
{{ generate_op_impl("scatter", smi_scatter.smi_scatter_impl) }}
// Gather
{{ generate_op_impl("gather", smi_gather.smi_gather_kernel) }}
{{ generate_op_impl("gather", smi_gather.smi_gather_channel) }}
{{ generate_op_impl("gather", smi_gather.smi_gather_impl) }}
// Reduce
{{ generate_op_impl("reduce", smi_reduce.smi_reduce_kernel) }}
{{ generate_op_impl("reduce", smi_reduce.smi_reduce_channel) }}
{{ generate_op_impl("reduce", smi_reduce.smi_reduce_impl) }}

{% import 'kernel.cl' as kernel %}

// QSFP channels
{% for channel in channels %}
channel SMI_NetworkMessage io_out_{{ channel.index }} __attribute__((depth(16))) __attribute__((io("kernel_output_ch{{ channel.index }}")));
channel SMI_NetworkMessage io_in_{{ channel.index }} __attribute__((depth(16))) __attribute__((io("kernel_input_ch{{ channel.index }}")));
{% endfor %}

// internal routing tables
__constant char internal_sender_rt[{{ tag_count }}] = { {{ range(tag_count)|join(", ") }} };
__constant char internal_receiver_rt[{{ tag_count }}] = { {{ range(tag_count)|join(", ") }} };

channel SMI_NetworkMessage channels_to_ck_s[{{ tag_count }}] __attribute__((depth(16)));
channel SMI_NetworkMessage channels_from_ck_r[{{ tag_count }}] __attribute__((depth(16)));

__constant char QSFP_COUNT = {{ channels_per_fpga }};

// connect all CK_S together
channel SMI_NetworkMessage channels_interconnect_ck_s[QSFP_COUNT*(QSFP_COUNT-1)] __attribute__((depth(16)));

// connect all CK_R together
channel SMI_NetworkMessage channels_interconnect_ck_r[QSFP_COUNT*(QSFP_COUNT-1)] __attribute__((depth(16)));

// connect corresponding CK_S/CK_R pairs
channel SMI_NetworkMessage channels_interconnect_ck_s_to_ck_r[QSFP_COUNT] __attribute__((depth(16)));

// connect corresponding CK_R/CK_S pairs
channel SMI_NetworkMessage channels_interconnect_ck_r_to_ck_s[QSFP_COUNT] __attribute__((depth(16)));


{% for channel in channels %}
{{ kernel.cks(channel, rank_count, channels|length, tag_count, target_index, tags_per_channel) }}
{{ kernel.ckr(channel, rank_count, channels|length, tag_count, target_index, tags_per_channel) }}
{% endfor %}

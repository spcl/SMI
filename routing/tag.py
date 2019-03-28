from common import FPGA


def assign_tags(fpga: FPGA, tag_count: int):
    channel_index = 0
    for tag in range(tag_count):
        fpga.channels[channel_index].tags.append(tag)
        channel_index = (channel_index + 1) % len(fpga.channels)

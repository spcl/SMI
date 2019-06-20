from typing import List

import bitstring

from program import Channel, FPGA

CKS_TARGET_QSFP = 0
CKS_TARGET_CKR = 1


class NoRouteFound(BaseException):
    pass


def closest_path_to_fpga(paths, channel: Channel, target: FPGA):
    routes = paths[channel]
    connections = []
    for destination in routes:
        if destination.fpga == target:
            connections.append(routes[destination])

    if not connections:
        raise NoRouteFound("No route found from {} to {}".format(channel, target))
    return min(connections, key=lambda c: len(c))


def get_output_target(paths, channel: Channel, target: FPGA):
    """
    0 -> local QSFP
    1 -> CK_R
    2 -> first neighbour
    3 -> second neighbour
    4 -> ...
    """
    if target == channel.fpga:
        return CKS_TARGET_CKR

    path = closest_path_to_fpga(paths, channel, target)[1:]  # skip the channel itself
    if path[0].fpga == channel.fpga:
        return 2 + channel.target_index(path[0].index)
    else:
        return CKS_TARGET_QSFP


def cks_routing_table(paths, fpgas: List[FPGA], channel: Channel) -> List[int]:
    table = []
    for fpga in fpgas:
        target = get_output_target(paths, channel, fpga)
        table.append(target)
    return table


def get_input_target(channel: Channel, tag: int, channels_per_fpga: int) -> int:
    """
    0 -> local CK_S (never generated here)
    1 -> CK_R_0
    2 -> CK_R_1
    ...
    [channels_per_fpga - 1] -> CK_R_N-1
    N -> app 0 on the given channel
    N + 1 -> app 1 on the given channel
    """
    target_channel_index = tag % channels_per_fpga
    if target_channel_index == channel.index:
        return channels_per_fpga + tag // channels_per_fpga
    else:
        return 1 + channel.target_index(target_channel_index)


def ckr_routing_table(channel: Channel, channels_per_fpga: int, tag_count: int) -> List[int]:
    table = []
    for tag in range(tag_count):
        table.append(get_input_target(channel, tag, channels_per_fpga))
    return table


def serialize_to_array(table: List[int], bytes=1):
    stream = bitstring.BitStream()
    bitcount = bytes * 8
    for target in table:
        stream.append("uintle:{}={}".format(bitcount, target))
    return stream.bytes

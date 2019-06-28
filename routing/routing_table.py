from typing import List

import bitstring

from ops import KEY_CKR_DATA, KEY_CKR_CONTROL
from program import Channel, FPGA, Program, INVALID_HARDWARE_PORT, split_kernel_method

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


def get_input_target(channel: Channel, logical_port: int, program: Program,
                     channels_per_fpga: int, key) -> int:
    """
    0 -> local CK_S (never generated here)
    1 -> CK_R_0
    2 -> CK_R_1
    ...
    [channels_per_fpga - 1] -> CK_R_N-1
    N -> first hardware port assigned to the given channel
    N + 1 -> second hardware port assigned to the given channel
    """
    group = program.create_group(key)
    hw_mapping = group.hw_mapping()

    hw_port = hw_mapping[logical_port]
    if hw_port == INVALID_HARDWARE_PORT:
        return 0

    target_channel_index = program.get_channel_for_logical_port(logical_port, key)
    if target_channel_index != channel.index:
        return 1 + channel.target_index(target_channel_index)

    kernel, method = split_kernel_method(key)
    return channels_per_fpga + program.get_channel_allocations(channel.index)[kernel].index((method, logical_port, hw_port))


def ckr_routing_table(channel: Channel, channels_per_fpga: int, program: Program) -> List[int]:
    table = []
    for port in range(program.logical_port_count):
        table.append(get_input_target(channel, port, program, channels_per_fpga, KEY_CKR_DATA))
        table.append(get_input_target(channel, port, program, channels_per_fpga, KEY_CKR_CONTROL))
    return table


def serialize_to_array(table: List[int], bytes=1):
    stream = bitstring.BitStream()
    bitcount = bytes * 8
    for target in table:
        stream.append("uintle:{}={}".format(bitcount, target))
    return stream.bytes

from typing import List

import bitstring

from ops import KEY_CKR_CONTROL, KEY_CKR_DATA
from program import Channel, FPGA, Program

CKS_TARGET_QSFP = 0
CKS_TARGET_CKR = 1


class NoRouteFound(BaseException):
    pass


class RoutingTable:
    def serialize(self, bytes=1) -> bytes:
        stream = bitstring.BitStream()
        bitcount = bytes * 8
        for target in self.get_data():
            stream.append("uintle:{}={}".format(bitcount, target))
        return stream.bytes

    def get_data(self):
        raise NotImplementedError()


class CKSRoutingTable(RoutingTable):
    def __init__(self, data):  # max_ranks, max_ports):
        # self.ranks = [[None] * max_ports for _ in range(max_ranks)]
        self.data = data

    def get_data(self):
        return self.data


class CKRRoutingTable(RoutingTable):
    def __init__(self, data: List[int]):
        self.data = data

    def get_data(self):
        return self.data


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


def cks_routing_table(paths, fpgas: List[FPGA], channel: Channel) -> CKSRoutingTable:
    table = []
    for fpga in fpgas:
        target = get_output_target(paths, channel, fpga)
        table.append(target)
    return CKSRoutingTable(table)


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

    target_channel_index = program.get_channel_for_port_key(logical_port, key)
    if target_channel_index is None:
        return 0
    if target_channel_index != channel.index:
        return 1 + channel.target_index(target_channel_index)

    allocations = tuple((op.logical_port, key) for (op, key)
                        in program.get_channel_allocations_with_prefix(channel.index, "ckr"))
    return channels_per_fpga + allocations.index((logical_port, key))


def ckr_routing_table(channel: Channel, channels_per_fpga: int,
                      program: Program) -> CKRRoutingTable:
    table = []
    for port in range(program.logical_port_count):
        table.append(get_input_target(channel, port, program, channels_per_fpga, KEY_CKR_DATA))
        table.append(get_input_target(channel, port, program, channels_per_fpga, KEY_CKR_CONTROL))
    return CKRRoutingTable(table)

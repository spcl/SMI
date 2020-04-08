import itertools
from typing import Dict, List, Tuple, Union

import bitstring

from ops import KEY_CKR_CONTROL, KEY_CKR_DATA
from program import Channel, FPGA, Program

CKS_TARGET_QSFP = 0
CKS_TARGET_CKR = 1


class NoRouteFound(BaseException):
    pass


class RoutingTarget:
    def serialize(self):
        raise NotImplementedError()

    def __eq__(self, other):
        return isinstance(other, self.__class__)


class CKRTarget(RoutingTarget):
    def serialize(self):
        return CKS_TARGET_CKR

    def __repr__(self):
        return "CKR"


class QSFPTarget(RoutingTarget):
    def serialize(self):
        return CKS_TARGET_QSFP

    def __repr__(self):
        return "QSFP"


class CKSTarget(RoutingTarget):
    def __init__(self, source: Channel, target: Channel):
        assert source.fpga == target.fpga
        self.source = source
        self.target = target

    def serialize(self):
        return 2 + self.source.target_index(self.target.index)

    def __eq__(self, other):
        return super().__eq__(other) and (self.source, self.target) == (other.source, other.target)

    def __repr__(self):
        fpga = self.source.fpga.key()
        return f"{fpga}:{self.source.index} -> {self.target.index}"


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
    def __init__(self, max_ranks, max_ports):
        self.data: List[List[RoutingTarget]] = [[QSFPTarget] * max_ports for _ in range(max_ranks)]

    def set_target(self, rank: int, port: int, target: RoutingTarget):
        self.data[rank][port] = target

    def get_target(self, rank: int, port: int) -> RoutingTarget:
        return self.data[rank][port]

    def get_data(self):
        return [target.serialize() for target in itertools.chain.from_iterable(self.data)]


class CKRRoutingTable(RoutingTable):
    def __init__(self, data: List[int]):
        self.data = data

    def get_data(self):
        return self.data


def path_fpga_length(path):
    fpgas = set()
    for channel in path:
        fpgas.add(channel.fpga)
    return len(fpgas)


def items_with_minimum(items, cost_fn, sort_fn=None):
    if sort_fn is None:
        sort_fn = cost_fn

    assert len(items) > 0
    sorted_items = sorted(items, key=sort_fn)
    minimum = cost_fn(sorted_items[0])
    return [item for item in sorted_items if cost_fn(item) == minimum]


def get_connections(paths, channel: Channel, target: FPGA):
    routes = paths[channel]
    connections = []
    for destination in routes:
        if destination.fpga == target:
            connections.append(routes[destination])

    if not connections:
        raise NoRouteFound("No route found from {} to {}".format(channel, target))
    return connections


def closest_path_to_fpga(paths, channel: Channel, target: FPGA):
    connections = get_connections(paths, channel, target)
    return min(connections, key=lambda c: len(c))


def get_output_target(paths, channel: Channel, target: FPGA) -> RoutingTarget:
    """
    0 -> local QSFP
    1 -> CK_R
    2 -> first neighbour
    3 -> second neighbour
    4 -> ...
    """
    if target == channel.fpga:
        return CKRTarget()

    path = closest_path_to_fpga(paths, channel, target)[1:]  # skip the channel itself
    if path[0].fpga == channel.fpga:
        return CKSTarget(channel, path[0])
    else:
        return QSFPTarget()


def closest_paths_to_fpga(paths, channel: Channel, target: FPGA):
    connections = get_connections(paths, channel, target)
    result = items_with_minimum(connections, lambda c: path_fpga_length(c),
                                lambda c: (path_fpga_length(c), len(c)))
    return [r[1:] for r in result]


def get_output_target_balanced(paths, channel: Channel, target: FPGA, occupancy) -> Tuple[
    RoutingTarget, Union[Channel, None]]:
    if target == channel.fpga:
        return (CKRTarget(), None)

    closest_paths = closest_paths_to_fpga(paths, channel, target)
    channel_candidates = {}
    for path in closest_paths:
        assert len(path) > 0
        if path[0].fpga != channel.fpga:
            candidate = channel
        else:
            candidate = path[0]
        if candidate not in channel_candidates:
            channel_candidates[candidate] = (occupancy[candidate], [])
        channel_candidates[candidate][1].append(path)
    best_channels = items_with_minimum(channel_candidates.items(), lambda c: c[1][0],
                                       lambda c: (c[1][0], min(len(p) for p in c[1][1])))
    target_channel = best_channels[0][0]
    assert target_channel.fpga == channel.fpga
    if target_channel == channel:
        return (QSFPTarget(), target_channel)
    else:
        return (CKSTarget(channel, target_channel), target_channel)


def cks_routing_tables(fpga: FPGA, fpgas: List[FPGA], paths) -> Dict[Channel, CKSRoutingTable]:
    program = fpga.program
    occupancy = {
        channel: 0
        for channel in fpga.channels
    }
    tables = {
        channel: CKSRoutingTable(len(fpgas), program.logical_port_count)
        for channel in fpga.channels
    }
    for target_fpga in fpgas:
        # calculate routes based on shortest paths, taking inter-CK hops into account
        for channel in fpga.channels:
            target = get_output_target(paths, channel, target_fpga)
            for port in range(program.logical_port_count):
                tables[channel].set_target(target_fpga.rank, port, target)

        # balance QSFPs, ignoring inter-CK hops
        for channel in fpga.channels:
            for (op, _) in program.get_channel_allocations_with_prefix(channel.index, "cks"):
                target, occupy_channel = get_output_target_balanced(paths, channel, target_fpga,
                                                                    occupancy)
                tables[channel].set_target(target_fpga.rank, op.logical_port, target)
                if occupy_channel is not None:
                    occupancy[occupy_channel] += 1
    return tables


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

from typing import Dict

import bitstring

from common import CHANNELS_PER_FPGA, Channel


CKS_TARGET_QSFP = 0
CKS_TARGET_CKR = 1


class NoRouteFound(BaseException):
    pass


def closest_path_to_rank(paths, channel: Channel, target_rank: str):
    routes = paths[channel]
    connections = []
    for destination in routes:
        if destination.fpga_key() == target_rank:
            connections.append(routes[destination])

    if not connections:
        raise NoRouteFound("No route found from {} to {}".format(channel, target_rank))
    return min(connections, key=lambda c: len(c))


def is_local(channel_a: Channel, channel_b: Channel):
    return channel_a.fpga_key() == channel_b.fpga_key()


def get_output_target(paths, channel, target_fpga: str):
    """
    0 -> local QSFP
    1 -> CK_R
    2 -> first neighbour
    3 -> second neighbour
    4 -> ...
    """
    if target_fpga == channel.fpga_key():
        return CKS_TARGET_CKR

    path = closest_path_to_rank(paths, channel, target_fpga)[1:]  # skip the channel itself
    if is_local(path[0], channel):
        return 1 + (((path[0].index + CHANNELS_PER_FPGA) - channel.index) % CHANNELS_PER_FPGA)
    else:
        return CKS_TARGET_QSFP


def ckr_routing_table(paths, ranks: Dict[int, str], channel: Channel):
    table = []
    for r in ranks:
        target = get_output_target(paths, channel, ranks[r])
        table.append(target)
    return table


def serialize_to_array(table, bytes=1):
    stream = bitstring.BitStream()
    bitcount = bytes * 8
    for target in table:
        stream.append("int:{}={}".format(bitcount, target))
    return stream.bytes

from typing import List

import bitstring

from common import CHANNELS_PER_FPGA, Channel, FPGA

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
        return 1 + (((path[0].index + CHANNELS_PER_FPGA) - channel.index) % CHANNELS_PER_FPGA)
    else:
        return CKS_TARGET_QSFP


def ckr_routing_table(paths, fpgas: List[FPGA], channel: Channel) -> List[int]:
    table = []
    for fpga in fpgas:
        target = get_output_target(paths, channel, fpga)
        table.append(target)
    return table


def cks_routing_table(paths, fpgas: List[FPGA], channel: Channel) -> List[int]:
    table = []
    for index, _ in enumerate(fpgas):
        table.append(index)
    return table


def serialize_to_array(table: List[int], bytes=1):
    stream = bitstring.BitStream()
    bitcount = bytes * 8
    for target in table:
        stream.append("int:{}={}".format(bitcount, target))
    return stream.bytes

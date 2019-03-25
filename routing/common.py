from typing import List

from networkx import Graph

COST_INTER_FPGA = 100
COST_INTRA_FPGA = 1
CHANNELS_PER_FPGA = 4


def target_index(source: int, target: int) -> int:
    """Returns the index of the target channel, skipping the index of the self channel."""
    assert source != target
    if target < source:
        return target
    return target - 1


class Channel:
    def __init__(self, fpga: "FPGA", index: int):
        self.fpga = fpga
        self.index = index

    def target_index(self, target: int) -> int:
        return target_index(self.index, target)

    def neighbours(self):
        for i in range(CHANNELS_PER_FPGA):
            if i != self.index:
                yield i

    def __repr__(self):
        return "Channel({}:{})".format(self.fpga, self.index)


class FPGA:
    def __init__(self, node: str, name: str):
        self.node = node
        self.name = name
        self.channels: List[Channel] = [Channel(self, i) for i in range(CHANNELS_PER_FPGA)]
        self.rank = None

    def has_channel(self, index: int):
        return self.channels[index] is not None

    def add_channel(self, index):
        assert not self.has_channel(index)
        self.channels[index] = Channel(self, index)

    def key(self):
        return "{}:{}".format(self.node, self.name)

    def __repr__(self):
        return "FPGA({})".format(self.key())


class RoutingContext:
    def __init__(self, graph: Graph, routes, fpgas: List[FPGA]):
        self.graph = graph
        self.routes = routes
        self.fpgas = fpgas


def write_nodefile(fpgas: List[FPGA], stream):
    fpgas = sorted(fpgas, key=lambda f: f.rank)

    for fpga in fpgas:
        stream.write("{}  # {}\n".format(fpga.node, fpga.name))

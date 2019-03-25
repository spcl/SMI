from typing import List

from networkx import Graph

COST_INTER_FPGA = 100
COST_INTRA_FPGA = 1
CHANNELS_PER_FPGA = 4


class Channel:
    def __init__(self, fpga: "FPGA", index: int):
        self.fpga = fpga
        self.index = index

    def next_index(self):
        return (self.index + 1) % CHANNELS_PER_FPGA

    def __repr__(self):
        return "Channel({}:{})".format(self.fpga, self.index)


class FPGA:
    def __init__(self, node: str, name: str):
        self.node = node
        self.name = name
        self.channels: List[Channel] = [None] * CHANNELS_PER_FPGA
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

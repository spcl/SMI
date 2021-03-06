from typing import List

from networkx import Graph

from program import FPGA


class RoutingContext:
    def __init__(self, graph: Graph, routes, fpgas: List[FPGA]):
        self.graph = graph
        self.routes = routes
        self.fpgas = fpgas


def write_nodefile(fpgas: List[FPGA], stream):
    fpgas = sorted(fpgas, key=lambda f: f.rank)

    for (index, fpga) in enumerate(fpgas):
        stream.write("{}  # {}, rank{}\n".format(fpga.node, fpga.name, index))

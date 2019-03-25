import re
from typing import List

import networkx

from common import COST_INTRA_FPGA, COST_INTER_FPGA, Channel, FPGA, RoutingContext

"""
Each CK_R/CK_S separate QSFP
CK_S <-> CK_R interconnected
CK_R/CK_S connected only to a single neighbour CKR/CK_S

fpga-0014:acl0:ch0 - fpga-0014:acl1:ch0
"""


def create_routing_context(stream):
    graph = networkx.Graph()
    fpgas = load_inter_fpga_connections(graph, stream)
    add_intra_fpga_connections(graph)
    routes = shortest_paths(graph)
    fpgas = create_ranks_for_fpgas(fpgas)
    return RoutingContext(graph, routes, fpgas)


def load_inter_fpga_connections(graph, stream) -> List[FPGA]:
    fpgas = {}
    channel_regex = re.compile(r".*(\d+)$")

    def parse_channel(data):
        node, fpga_name, channel = data.split(":")
        fpga_key = "{}:{}".format(node, fpga_name)
        if fpga_key not in fpgas:
            fpgas[fpga_key] = FPGA(node, fpga_name)
        fpga = fpgas[fpga_key]

        match = channel_regex.match(channel)
        index = int(match.group(1))
        if not fpga.has_channel(index):
            fpga.add_channel(index)
        return fpga.channels[index]

    for line in stream:
        line = line.strip()
        if line:
            src, dst = [parse_channel(l.strip()) for l in line.split("<->")]
            graph.add_edge(src, dst, weight=COST_INTER_FPGA)

    return list(fpgas.values())


def add_intra_fpga_connections(graph):
    for a in graph.nodes:
        for b in graph.nodes:
            if a is not b and a.fpga == b.fpga:
                graph.add_edge(a, b, weight=COST_INTRA_FPGA)


def shortest_paths(graph):
    return networkx.shortest_path(graph, source=None, target=None, weight="weight")


def create_ranks_for_fpgas(fpgas: List[FPGA]) -> List[FPGA]:
    """
    Enumerates all channels and assigns ranks to individual FPGAs, sorted by their (node, fpga)
    name pair.
    """
    fpgas = sorted(fpgas, key=lambda f: f.key())
    for (rank, fpga) in enumerate(fpgas):
        fpga.rank = rank
    return fpgas

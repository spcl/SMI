import re
from typing import Dict

import networkx

from common import COST_INTRA_FPGA, COST_INTER_FPGA, Channel

"""
Each CK_R/CK_S separate QSFP
CK_S <-> CK_R interconnected
CK_R/CK_S connected only to a single neighbour CKR/CK_S

fpga-0014:acl0:ch0 - fpga-0014:acl1:ch0
"""


class RoutingContext:
    def __init__(self, graph, routes, ranks):
        self.graph = graph
        self.routes = routes
        self.ranks = ranks


def create_routing_context(stream):
    graph = networkx.Graph()
    load_inter_fpga_connections(graph, stream)
    add_intra_fpga_connections(graph)
    routes = shortest_paths(graph)
    return RoutingContext(graph, routes, create_ranks_for_fpgas(graph))


def load_inter_fpga_connections(graph, stream):
    channels = {}
    channel_regex = re.compile(r".*(\d+)$")

    def parse_channel(data):
        node, fpga, channel = data.split(":")
        match = channel_regex.match(channel)
        index = int(match.group(1))
        if data not in channels:
            channels[data] = Channel(node, fpga, index)
        return channels[data]

    for line in stream:
        line = line.strip()
        if line:
            src, dst = [parse_channel(l.strip()) for l in line.split(" - ")]
            graph.add_edge(src, dst, weight=COST_INTER_FPGA)


def add_intra_fpga_connections(graph):
    for a in graph.nodes:
        for b in graph.nodes:
            if a is not b and a.fpga_key() == b.fpga_key():
                graph.add_edge(a, b, weight=COST_INTRA_FPGA)


def shortest_paths(graph):
    return networkx.shortest_path(graph, source=None, target=None, weight="weight")


def create_ranks_for_fpgas(graph: networkx.Graph) -> Dict[int, str]:
    nodes = set()
    for channel in graph.nodes:
        nodes.add(channel.fpga_key())
    nodes = sorted(nodes)
    return dict(enumerate(nodes))

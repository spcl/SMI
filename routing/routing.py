from typing import List, Tuple, Dict

import networkx
from networkx import Graph

from common import COST_INTRA_FPGA, COST_INTER_FPGA, FPGA, RoutingContext
from program import ProgramMapping

"""
Each CK_R/CK_S separate QSFP
CK_S <-> CK_R interconnected
CK_R/CK_S connected only to a single neighbour CKR/CK_S

fpga-0014:acl0:ch0 - fpga-0014:acl1:ch0
"""


def create_routing_context(fpga_connections: Dict[Tuple[str, int], Tuple[str, int]], program_mapping: ProgramMapping):
    graph = networkx.Graph()
    fpgas = load_inter_fpga_connections(graph, fpga_connections, program_mapping)
    add_intra_fpga_connections(graph, fpgas)
    routes = shortest_paths(graph)
    fpgas = create_ranks_for_fpgas(fpgas)
    return RoutingContext(graph, routes, fpgas)


def load_inter_fpga_connections(graph: networkx.Graph,
                                fpga_connections: Dict[Tuple[str, int], Tuple[str, int]],
                                program_mapping: ProgramMapping) -> List[FPGA]:
    """
    Parses FPGA connections and embeds them into a graph.
    """
    fpgas = {}

    def get_channel(fpga_key, channel):
        if fpga_key not in fpgas:
            node, fpga_name = fpga_key.split(":")
            fpgas[fpga_key] = FPGA(node, fpga_name, program_mapping.fpga_map[fpga_key])
        fpga = fpgas[fpga_key]
        return fpga.channels[channel]

    for (src, dst) in fpga_connections.items():
        src, dst = [get_channel(p[0], p[1]) for p in (src, dst)]
        graph.add_edge(src, dst, weight=COST_INTER_FPGA, label="{}-{}".format(src, dst))

    return list(fpgas.values())


def add_intra_fpga_connections(graph: Graph, fpgas: List[FPGA]):
    for fpga in fpgas:
        for a in fpga.channels:
            for b in fpga.channels:
                if a is not b:
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

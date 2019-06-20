import networkx

from parser import parse_fpga_connections
from program import ProgramMapping, Program
from routing import load_inter_fpga_connections, create_routing_context
from utils import round_robin


def test_round_robin():
    assert round_robin([], 0, 1) == []
    assert round_robin([1], 0, 1) == [1]
    assert round_robin([1], 1, 2) == []
    assert round_robin([1, 2], 0, 2) == [1]
    assert round_robin([1, 2], 1, 2) == [2]
    assert round_robin([1, 2, 3, 4, 5, 6, 7], 1, 4) == [2, 6]

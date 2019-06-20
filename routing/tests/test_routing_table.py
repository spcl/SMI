import pytest
from conftest import init_graph, get_channel

from table import cks_routing_table, NoRouteFound


def test_cks_table():
    tag_count = 9
    fpga = FPGA("n", "f")

    assert ckr_routing_table(fpga.channels[0], CHANNELS_PER_FPGA, tag_count) == [4, 1, 2, 3, 5, 1, 2, 3, 6]


def test_cks_table():
    ctx = init_graph("""
    N0:F0:ch0 <-> N0:F1:ch0
    N1:F0:ch0 <-> N0:F0:ch1
""")

    graph, routes, fpgas = (ctx.graph, ctx.routes, ctx.fpgas)

    a = get_channel(graph, "N0:F0", 0)
    assert cks_routing_table(routes, fpgas, a) == [1, 0, 2]

    b = get_channel(graph, "N0:F0", 1)
    assert cks_routing_table(routes, fpgas, b) == [1, 2, 0]

    c = get_channel(graph, "N0:F1", 0)
    assert cks_routing_table(routes, fpgas, c) == [0, 1, 0]

    d = get_channel(graph, "N1:F0", 0)
    assert cks_routing_table(routes, fpgas, d) == [0, 0, 1]


def test_ckr_no_route():
    ctx = init_graph("""
    N0:F0:ch0 <-> N0:F1:ch0
    N1:F0:ch0 <-> N1:F2:ch1
""")
    graph, routes, fpgas = (ctx.graph, ctx.routes, ctx.fpgas)
    ch = get_channel(graph, "N0:F0", 0)
    with pytest.raises(NoRouteFound):
        cks_routing_table(routes, fpgas, ch)

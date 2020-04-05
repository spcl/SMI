import pytest
from conftest import get_routing_ctx, get_channel

from ops import Push, Pop
from program import FPGA, Program, CHANNELS_PER_FPGA
from routing_table import cks_routing_table, NoRouteFound, ckr_routing_table


def test_cks_table():
    ctx = get_routing_ctx(Program([
        Push(0),
        Push(1),
        Pop(2)
    ]), {
        ("N0:F0", 0): ("N0:F1", 0),
        ("N1:F0", 0): ("N0:F0", 1)
    })

    graph, routes, fpgas = (ctx.graph, ctx.routes, ctx.fpgas)

    a = get_channel(graph, "N0:F0", 0)
    assert cks_routing_table(routes, fpgas, a).get_data() == [1, 0, 2]

    b = get_channel(graph, "N0:F0", 1)
    assert cks_routing_table(routes, fpgas, b).get_data() == [1, 2, 0]

    c = get_channel(graph, "N0:F1", 0)
    assert cks_routing_table(routes, fpgas, c).get_data() == [0, 1, 0]

    d = get_channel(graph, "N1:F0", 0)
    assert cks_routing_table(routes, fpgas, d).get_data() == [0, 0, 1]


def test_ckr_table():
    program = Program([
        Push(0),
        Pop(1),
        Push(2),
        Pop(3),
        Pop(4)
    ])
    fpga = FPGA("n", "f", program)

    def get_table(index):
        return ckr_routing_table(fpga.channels[index], CHANNELS_PER_FPGA, program).get_data()

    assert get_table(0) == [0, 3, 4, 0, 0, 5, 1, 0, 2, 0]
    assert get_table(1) == [0, 3, 1, 0, 0, 1, 4, 0, 2, 0]
    assert get_table(2) == [0, 3, 1, 0, 0, 1, 2, 0, 4, 0]
    assert get_table(3) == [0, 4, 1, 0, 0, 1, 2, 0, 3, 0]


def test_ckr_no_route():
    ctx = get_routing_ctx(Program([]), {
        ("N0:F0", 0): ("N0:F1", 0),
        ("N1:F0", 0): ("N1:F2", 1)
    })

    graph, routes, fpgas = (ctx.graph, ctx.routes, ctx.fpgas)
    ch = get_channel(graph, "N0:F0", 0)
    with pytest.raises(NoRouteFound):
        cks_routing_table(routes, fpgas, ch)

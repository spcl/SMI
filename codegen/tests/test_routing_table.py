import pytest
from conftest import get_fpga, get_routing_ctx

from ops import Pop, Push
from program import CHANNELS_PER_FPGA, FPGA, Program
from routing_table import NoRouteFound, ckr_routing_table, cks_routing_tables


def test_cks_table_1():
    program = Program([
        Push(0),
        Push(1)
    ])
    ctx = get_routing_ctx(program, {
        ("N0:FA", 1): ("N0:FB", 1),
        ("N0:FA", 3): ("N0:FB", 3)
    })

    graph, routes, fpgas = (ctx.graph, ctx.routes, ctx.fpgas)

    fa = get_fpga(fpgas, "N0:FA")
    tables = cks_routing_tables(fa, fpgas, routes)
    assert tables[fa.channels[0]].get_data() == [1, 1, 2, 2]
    assert tables[fa.channels[1]].get_data() == [1, 1, 0, 4]
    assert tables[fa.channels[2]].get_data() == [1, 1, 3, 3]
    assert tables[fa.channels[3]].get_data() == [1, 1, 0, 0]


def test_cks_table_2():
    program = Program([
        Push(0),
        Push(1)
    ])
    ctx = get_routing_ctx(program, {
        ("N0:FA", 0): ("N0:FB", 0),
        ("N0:FA", 3): ("N0:FB", 3)
    })

    graph, routes, fpgas = (ctx.graph, ctx.routes, ctx.fpgas)

    fa = get_fpga(fpgas, "N0:FA")
    tables = cks_routing_tables(fa, fpgas, routes)
    assert tables[fa.channels[0]].get_data() == [1, 1, 0, 0]
    assert tables[fa.channels[1]].get_data() == [1, 1, 2, 4]
    assert tables[fa.channels[2]].get_data() == [1, 1, 2, 2]
    assert tables[fa.channels[3]].get_data() == [1, 1, 0, 0]


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


def test_cks_no_route():
    ctx = get_routing_ctx(Program([]), {
        ("N0:F0", 0): ("N0:F1", 0),
        ("N1:F0", 0): ("N1:F2", 1)
    })

    graph, routes, fpgas = (ctx.graph, ctx.routes, ctx.fpgas)
    fa = get_fpga(fpgas, "N0:F0")
    with pytest.raises(NoRouteFound):
        cks_routing_tables(fa, fpgas, routes)

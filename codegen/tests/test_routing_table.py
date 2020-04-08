import pytest
from conftest import get_fpga, get_routing_ctx

from ops import Pop, Push
from program import CHANNELS_PER_FPGA, FPGA, Program
from routing_table import NoRouteFound, ckr_routing_table, \
    cks_routing_tables


def check_fpga_tables(fpga, tables, expected):
    assert len(tables) == len(expected)
    assert len(tables) == CHANNELS_PER_FPGA

    for (index, channel) in enumerate(fpga.channels):
        assert [[repr(v) for v in l] for l in tables[channel].data] == expected[index]


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
    check_fpga_tables(fa, tables, [
        [["CKR", "CKR"], ["0->1", "0->1"]],
        [["CKR", "CKR"], ["QSFP", "1->3"]],
        [["CKR", "CKR"], ["2->1", "2->1"]],
        [["CKR", "CKR"], ["QSFP", "QSFP"]]
    ])


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
    check_fpga_tables(fa, tables, [
        [["CKR", "CKR"], ["QSFP", "QSFP"]],
        [["CKR", "CKR"], ["1->0", "1->3"]],
        [["CKR", "CKR"], ["2->0", "2->0"]],
        [["CKR", "CKR"], ["QSFP", "QSFP"]]
    ])


def test_cks_table_double_rail():
    program = Program([
        Push(0),
        Pop(0),
        Push(1),
        Pop(1)
    ])
    ctx = get_routing_ctx(program, {
        ("N1:F0", 1): ("N1:F1", 0),
        ("N1:F0", 3): ("N1:F1", 2),
        ("N1:F1", 1): ("N2:F0", 0),
        ("N1:F1", 3): ("N2:F0", 2),
        ("N2:F0", 1): ("N2:F1", 0),
        ("N2:F0", 3): ("N2:F1", 2),
        ("N2:F1", 1): ("N1:F0", 0),
        ("N2:F1", 3): ("N1:F0", 2),
    })

    graph, routes, fpgas = (ctx.graph, ctx.routes, ctx.fpgas)

    fpga = get_fpga(fpgas, "N1:F0")
    tables = cks_routing_tables(fpga, fpgas, routes)
    check_fpga_tables(fpga, tables, [
        [["CKR", "CKR"], ["0->1", "0->1"], ["QSFP", "QSFP"], ["0->2", "QSFP"]],
        [["CKR", "CKR"], ["QSFP", "1->3"], ["QSFP", "1->0"], ["1->0", "1->0"]],
        [["CKR", "CKR"], ["2->1", "2->1"], ["QSFP", "QSFP"], ["QSFP", "QSFP"]],
        [["CKR", "CKR"], ["QSFP", "QSFP"], ["QSFP", "QSFP"], ["3->0", "3->0"]]
    ])

    fpga = get_fpga(fpgas, "N1:F1")
    tables = cks_routing_tables(fpga, fpgas, routes)
    check_fpga_tables(fpga, tables, [
        [["QSFP", "QSFP"], ["CKR", "CKR"], ["0->1", "0->1"], ["QSFP", "QSFP"]],
        [["1->0", "1->2"], ["CKR", "CKR"], ["QSFP", "1->3"], ["QSFP", "QSFP"]],
        [["QSFP", "QSFP"], ["CKR", "CKR"], ["2->1", "2->1"], ["QSFP", "QSFP"]],
        [["3->0", "3->0"], ["CKR", "CKR"], ["QSFP", "QSFP"], ["QSFP", "QSFP"]]
    ])


def test_cks_table_double_rail2():
    program = Program([
        Push(0),
        Pop(0),
        Push(1),
        Pop(1)
    ])
    ctx = get_routing_ctx(program, {
        ("N:F0", 1): ("N:F1", 0),
        ("N:F0", 3): ("N:F1", 2),
        ("N:F1", 1): ("N:F2", 0),
        ("N:F1", 3): ("N:F2", 2),
        ("N:F2", 1): ("N:F3", 0),
        ("N:F2", 3): ("N:F3", 2),
        ("N:F3", 1): ("N:F4", 0),
        ("N:F3", 3): ("N:F4", 2),
        ("N:F4", 1): ("N:F5", 0),
        ("N:F4", 3): ("N:F5", 2),
        ("N:F5", 1): ("N:F0", 0),
        ("N:F5", 3): ("N:F0", 2),
    })

    graph, routes, fpgas = (ctx.graph, ctx.routes, ctx.fpgas)

    fpga = get_fpga(fpgas, "N:F4")
    tables = cks_routing_tables(fpga, fpgas, routes)
    check_fpga_tables(fpga, tables, [
        [['0->1', '0->1'], ['QSFP', 'QSFP'], ['QSFP', 'QSFP'], ['0->2', 'QSFP'], ['CKR', 'CKR'],
         ['0->3', '0->1']],
        [['QSFP', 'QSFP'], ['QSFP', '1->0'], ['1->0', '1->0'], ['1->0', '1->2'], ['CKR', 'CKR'],
         ['QSFP', 'QSFP']],
        [['2->1', '2->1'], ['QSFP', 'QSFP'], ['QSFP', 'QSFP'], ['QSFP', 'QSFP'], ['CKR', 'CKR'],
         ['2->3', '2->1']],
        [['QSFP', 'QSFP'], ['QSFP', 'QSFP'], ['3->0', '3->0'], ['3->0', '3->0'], ['CKR', 'CKR'],
         ['QSFP', 'QSFP']]
    ])


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

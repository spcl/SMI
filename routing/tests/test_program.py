import networkx

from parser import parse_fpga_connections
from program import ProgramMapping, Program, Push, Pop, Channel, FPGA
from routing import load_inter_fpga_connections, create_routing_context


def test_hardware_port_mapping():
    program = Program(4096, [
        Push(0),
        Pop(1),
        Push(2),
        Pop(3),
        Pop(4)
    ])

    assert program.logical_port_count() == 5
    assert program.hw_port_count() == 10

    # Push
    assert program.cks_data_mapping() == [0, -1, 1, -1, -1]
    assert program.ckr_control_mapping() == [3, -1, 4, -1, -1]

    # Pop
    assert program.ckr_data_mapping() == [-1, 0, -1, 1, 2]
    assert program.cks_control_mapping() == [-1, 2, -1, 3, 4]


def test_hardware_cks_port_allocation():
    program = Program(4096, [
        Push(0),
        Pop(1),
        Push(2),
        Pop(3),
        Pop(4)
    ])

    fpga = FPGA("", "", program)
    assert program.cks_hw_ports(Channel(fpga, 0), 2) == [0, 2, 4]
    assert program.cks_hw_ports(Channel(fpga, 1), 2) == [1, 3]


def test_hardware_ckr_port_allocation():
    program = Program(4096, [
        Push(0),
        Pop(1),
        Push(2),
        Pop(3),
        Pop(4)
    ])

    fpga = FPGA("", "", program)
    assert program.ckr_hw_ports(Channel(fpga, 0), 2) == [0, 2, 4]
    assert program.ckr_hw_ports(Channel(fpga, 1), 2) == [1, 3]

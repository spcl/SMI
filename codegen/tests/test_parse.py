from ops import Push, Pop, Broadcast, Reduce
from serialization import parse_program, parse_routing_file


def test_parse_program():
    program = parse_program("""
{
    "consecutive_reads": 16,
    "max_ranks": 16,
    "p2p_randezvous": false,
    "operations": [{
      "port": 0,
      "type": "push",
      "data_type": "int"
    }, {
      "port": 1,
      "type": "push",
      "data_type": "char"
    }, {
      "port": 2,
      "type": "pop"
    }, {
      "port": 3,
      "type": "broadcast",
      "data_type": "int"
    }, {
      "port": 4,
      "type": "reduce",
      "data_type": "float",
      "args": {
        "op_type": "add"
      }
    }, {
      "port": 5,
      "type": "scatter",
      "data_type": "short"
    }, {
      "port": 6,
      "type": "gather",
      "data_type": "double",
      "buffer_size": 32
    }]
}
""")
    assert program.consecutive_read_limit == 16
    assert program.max_ranks == 16
    assert len(program.operations) == 7
    assert isinstance(program.operations[0], Push)
    assert program.operations[0].logical_port == 0
    assert isinstance(program.operations[2], Pop)
    assert program.operations[2].logical_port == 2

    assert isinstance(program.operations[3], Broadcast)
    assert isinstance(program.operations[4], Reduce)
    assert program.operations[4].data_type == "float"

    assert program.operations[6].buffer_size == 32


def test_parse_connections():
    (connections, _) = parse_routing_file("""
{
    "fpgas": {},
    "connections": {
        "fpga-0015:acl0:ch0": "fpga-0016:acl0:ch0",
        "fpga-0015:acl0:ch1": "fpga-0015:acl1:ch1",
        "fpga-0015:acl0:ch2": "fpga-0016:acl1:ch2",
        "fpga-0015:acl1:ch0": "fpga-0016:acl1:ch0",
        "fpga-0015:acl1:ch2": "fpga-0016:acl0:ch2",
        "fpga-0016:acl0:ch1": "fpga-0016:acl1:ch1"
    }
}    
""", ignore_programs=True)
    assert connections == {('fpga-0015:acl0', 0): ('fpga-0016:acl0', 0),
                           ('fpga-0015:acl0', 1): ('fpga-0015:acl1', 1),
                           ('fpga-0015:acl0', 2): ('fpga-0016:acl1', 2),
                           ('fpga-0015:acl1', 0): ('fpga-0016:acl1', 0),
                           ('fpga-0015:acl1', 1): ('fpga-0015:acl0', 1),
                           ('fpga-0015:acl1', 2): ('fpga-0016:acl0', 2),
                           ('fpga-0016:acl0', 0): ('fpga-0015:acl0', 0),
                           ('fpga-0016:acl0', 1): ('fpga-0016:acl1', 1),
                           ('fpga-0016:acl0', 2): ('fpga-0015:acl1', 2),
                           ('fpga-0016:acl1', 0): ('fpga-0015:acl1', 0),
                           ('fpga-0016:acl1', 1): ('fpga-0016:acl0', 1),
                           ('fpga-0016:acl1', 2): ('fpga-0015:acl0', 2)}

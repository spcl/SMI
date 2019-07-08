from ops import Push, Pop, Broadcast, Reduce
from parser import parse_programs, parse_fpga_connections


def test_parse_programs():
    mapping = parse_programs("""
[{
    "buffer_size": 4096,
    "consecutive_reads": 16,
    "max_ranks": 16,
    "p2p_randezvous": false,
    "ports": [{
      "id": 0,
      "type": "push"
    }, {
      "id": 1,
      "type": "push"
    }, {
      "id": 2,
      "type": "pop"
    }, {
      "id": 3,
      "type": "broadcast"
    }, {
      "id": 4,
      "type": "reduce",
      "args": {
        "op_type": "add",
        "data_type": "float"
      }
    }, {
      "id": 5,
      "type": "scatter"
    }, {
      "id": 6,
      "type": "gather"
    }],
    "fpgas": ["fpga-0015", "fpga-0016"]
}, {
    "buffer_size": 8192,
    "ports": [{
      "id": 0,
      "type": "push"
    }],
    "fpgas": ["fpga-0014"]
}]
""")
    assert len(mapping.programs) == 2
    program = mapping.programs[0]
    assert program.buffer_size == 4096
    assert program.consecutive_read_limit == 16
    assert program.max_ranks == 16
    assert not program.p2p_randezvous
    assert len(program.operations) == 7
    assert isinstance(program.operations[0], Push)
    assert program.operations[0].logical_port == 0
    assert isinstance(program.operations[2], Pop)
    assert program.operations[2].logical_port == 2

    assert isinstance(program.operations[3], Broadcast)
    assert isinstance(program.operations[4], Reduce)
    assert program.operations[4].data_type == "float"

    assert mapping.fpga_map["fpga-0015"] is program
    assert mapping.fpga_map["fpga-0016"] is program

    program = mapping.programs[1]
    assert program.buffer_size == 8192
    assert len(program.operations) == 1
    assert isinstance(program.operations[0], Push)
    assert program.operations[0].logical_port == 0

    assert mapping.fpga_map["fpga-0014"] is program


def test_parse_connections():
    connections = parse_fpga_connections("""
fpga-0015:acl0:ch0 <-> fpga-0016:acl0:ch0
fpga-0015:acl0:ch1 <-> fpga-0015:acl1:ch1
fpga-0015:acl0:ch2 <-> fpga-0016:acl1:ch2
fpga-0015:acl1:ch0 <-> fpga-0016:acl1:ch0
fpga-0015:acl1:ch2 <-> fpga-0016:acl0:ch2
fpga-0016:acl0:ch1 <-> fpga-0016:acl1:ch1    
""")
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

from parser import parse_programs


def test_parse_programs():
    mapping = parse_programs("""
[{
    "buffer_size": 4096,
    "ports": [{
      "id": 0,
      "type": "p2p"
    }, {
      "id": 1,
      "type": "collective"
    }, {
      "id": 2,
      "type": "p2p"
    }],
    "fpgas": ["fpga-0015", "fpga-0016"]
}, {
    "buffer_size": 8192,
    "ports": [{
      "id": 0,
      "type": "collective"
    }],
    "fpgas": ["fpga-0014"]
}]
""")
    assert len(mapping.programs) == 2
    program = mapping.programs[0]
    assert program.buffer_size == 4096
    assert len(program.ports) == 3
    assert program.ports[0].id == 0
    assert program.ports[0].type == "p2p"
    assert program.ports[1].id == 1
    assert program.ports[1].type == "collective"

    assert mapping.fpga_map["fpga-0015"] is program
    assert mapping.fpga_map["fpga-0016"] is program

    program = mapping.programs[1]
    assert program.buffer_size == 8192
    assert len(program.ports) == 1
    assert program.ports[0].id == 0
    assert program.ports[0].type == "collective"

    assert mapping.fpga_map["fpga-0014"] is program

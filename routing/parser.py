import json
from typing import List

from program import ProgramMapping, Program, PortInfo


def parse_ports(ports) -> List[PortInfo]:
    return [PortInfo(int(p["id"]), p["type"]) for p in ports]


def parse_programs(input: str) -> ProgramMapping:
    """
    Parses programs from a JSON representation.
    """
    data = json.loads(input)
    programs = []
    fpga_mapping = {}

    for prog in data:
        program = Program(prog["buffer_size"], parse_ports(prog["ports"]))
        programs.append(program)
        for fpga in prog["fpgas"]:
            assert fpga not in fpga_mapping
            fpga_mapping[fpga] = program
    return ProgramMapping(programs, fpga_mapping)

import json
import re
from typing import List, Tuple, Dict

from ops import Broadcast, Push, Pop, Reduce, Scatter, Gather
from program import ProgramMapping, Program, SmiOperation


def parse_smi_operation(obj) -> SmiOperation:
    id = obj["id"]
    type = obj["type"]
    args = obj.get("args", {})

    types = {
        "push": Push,
        "pop": Pop,
        "broadcast": Broadcast,
        "reduce": Reduce,
        "scatter": Scatter,
        "gather": Gather
    }
    assert type in types
    return types[type](id, **args)


def parse_ports(ports) -> List[SmiOperation]:
    return [parse_smi_operation(p) for p in ports]


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


def parse_fpga_connections(input: str) -> Dict[Tuple[str, int], Tuple[str, int]]:
    channel_regex = re.compile(r".*(\d+)$")
    connections = {}

    def parse_key(data):
        node, fpga_name, channel = data
        return "{}:{}".format(node, fpga_name)

    def parse_channel(data):
        node, fpga_name, channel = data
        match = channel_regex.match(channel)
        return int(match.group(1))

    for line in input.splitlines():
        line = line.strip()
        if line:
            src, dst = [l.strip().split(":") for l in line.split("<->")]
            src, dst = [(parse_key(d), parse_channel(d)) for d in (src, dst)]
            assert src not in connections
            assert dst not in connections
            connections[src] = dst
            connections[dst] = src

    return connections

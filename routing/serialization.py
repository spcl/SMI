import json
import re
import os
from typing import List, Tuple, Dict

from ops import Broadcast, Push, Pop, Reduce, Scatter, Gather
from program import Program, SmiOperation, ProgramMapping

SMI_OP_KEYS = {
    "push": Push,
    "pop": Pop,
    "broadcast": Broadcast,
    "reduce": Reduce,
    "scatter": Scatter,
    "gather": Gather
}


def parse_smi_operation(obj) -> SmiOperation:
    type = obj["type"]
    port = obj["port"]
    data_type = obj.get("data_type", "int")
    buffer_size = obj.get("buffer_size")
    args = obj.get("args", {})

    assert type in SMI_OP_KEYS
    return SMI_OP_KEYS[type](port, data_type, buffer_size, **args)


def serialize_smi_operation(op: SmiOperation):
    inv_map = {v: k for k, v in SMI_OP_KEYS.items()}

    return {
        "type": inv_map[op.__class__],
        "port": op.logical_port,
        "data_type": op.data_type,
        "buffer_size": op.buffer_size,
        "args": op.serialize_args()
    }


def parse_operations(operations) -> List[SmiOperation]:
    return [parse_smi_operation(p) for p in operations]


def parse_program(input: str) -> Program:
    prog = json.loads(input)
    return Program(
        parse_operations(prog["operations"]),
        prog.get("consecutive_reads"),
        prog.get("max_ranks"),
        """prog.get("p2p_rendezvous") TODO: fix"""
    )


def serialize_program(program: Program) -> str:
    return json.dumps({
        "operations": [serialize_smi_operation(op) for op in program.operations]
    })


def parse_routing_file(data: str, metadata_paths=None, ignore_programs=False) -> Tuple[Dict[Tuple[str, int], Tuple[str, int]], ProgramMapping]:
    if metadata_paths is None:
        metadata_paths = []

    path_index = {}
    for path in metadata_paths:
        path_index[os.path.splitext(os.path.basename(path))[0]] = path

    data = json.loads(data)
    program_cache = {}
    fpga_map = {}
    for (fpga, program_path) in data.get("fpgas", {}).items():
        if program_path not in program_cache:
            if ignore_programs:
                program_cache[program_path] = None
            else:
                real_path = path_index[program_path]
                with open(real_path) as pf:
                    program_cache[program_path] = parse_program(pf.read())

        fpga_map[fpga] = program_cache[program_path]

    mapping = ProgramMapping(list(program_cache.values()), fpga_map)

    channel_regex = re.compile(r".*(\d+)$")
    connections = {}

    def parse_key(data):
        node, fpga_name, channel = data
        return "{}:{}".format(node, fpga_name)

    def parse_channel(data):
        node, fpga_name, channel = data
        match = channel_regex.match(channel)
        return int(match.group(1))

    for (src, dst) in data.get("connections", {}).items():
        src, dst = [item.split(":") for item in (src, dst)]
        src, dst = [(parse_key(d), parse_channel(d)) for d in (src, dst)]
        assert src not in connections
        assert dst not in connections
        connections[src] = dst
        connections[dst] = src

    return (connections, mapping)

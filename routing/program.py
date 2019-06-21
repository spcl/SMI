from typing import List, Dict, Tuple

from ops import SmiOperation
from utils import round_robin

COST_INTER_FPGA = 100
COST_INTRA_FPGA = 1
CHANNELS_PER_FPGA = 4

INVALID_HARDWARE_PORT = -1


class ChannelGroup:
    def __init__(self, logical_port_count: int, hw_ports, op_allocations, key):
        self.logical_port_count = logical_port_count
        self.hw_ports = dict(hw_ports)
        self.op_allocations = dict(op_allocations)
        self.key = key

    def hw_port_count(self) -> int:
        return self.hw_ports[self.key]

    def hw_mapping(self) -> List[int]:
        mapping = []

        for i in range(self.logical_port_count):
            if i in self.op_allocations and self.key in self.op_allocations[i]:
                mapping.append(self.op_allocations[i][self.key])
            else:
                mapping.append(INVALID_HARDWARE_PORT)
        return mapping


class FailedAllocation(Exception):
    def __init__(self, op: SmiOperation):
        self.op = op

    def __repr__(self):
        return "Allocation failed for op {}".format(self.op)


class Channel:
    def __init__(self, fpga: "FPGA", index: int):
        self.fpga = fpga
        self.index = index

    def target_index(self, target: int) -> int:
        return target_index(self.index, target)

    def neighbours(self):
        for i in range(CHANNELS_PER_FPGA):
            if i != self.index:
                yield i

    def __repr__(self):
        return "Channel({}:{})".format(self.fpga, self.index)


class Program:
    """
    Contains all information necessary to generate code for a single rank.

    Program has to provide:

    Globally:
    - number of logical ports
    - number of HW ports per cks/ckr x data/control
    For each channel:
    - allocated cks data/control
    - allocated ckr data/control
    For a logical port and data/control:
    - allocated channel
    """
    def __init__(self, buffer_size: int, operations: List[SmiOperation]):
        assert are_ops_consecutive(operations)

        self.buffer_size = buffer_size
        self.operations = sorted(operations, key=lambda op: op.logical_port)
        self.logical_port_count = max((op.logical_port for op in operations), default=0) + 1

        self.hardware_ports = {
            ("cks", "data"):    0,
            ("cks", "control"): 0,
            ("ckr", "data"):    0,
            ("ckr", "control"): 0
        }
        self.op_allocations = {}
        self.channel_allocations = {}

        for op in operations:
            self._allocate_op(op)
        self._allocate_channels(CHANNELS_PER_FPGA)

    def create_group(self, key: Tuple[str, str]) -> ChannelGroup:
        return ChannelGroup(self.logical_port_count, self.hardware_ports, self.op_allocations, key)

    def get_channel_allocations(self, channel: int):
        return self.channel_allocations[channel]

    def get_channel_for_logical_port(self, logical_port: int, key):
        hw_port = self.create_group(key).hw_mapping()[logical_port]
        if hw_port == INVALID_HARDWARE_PORT:
            return None

        (kernel, method) = key

        for (channel, kernels) in self.channel_allocations.items():
            allocations = kernels[kernel]
            for (allocated_method, allocated_hw_port) in allocations:
                if allocated_method == method and allocated_hw_port == hw_port:
                    return channel
        return None

    def _allocate_op(self, op: SmiOperation):
        logical_port = op.logical_port
        mapping = self.op_allocations.setdefault(logical_port, {})

        port_usage = op.hw_port_usage()
        for key in self.hardware_ports:
            if key in port_usage:
                if key in mapping:
                    raise FailedAllocation(op)
                mapping[key] = self.hardware_ports[key]
                self.hardware_ports[key] += 1

    def _allocate_channels(self, count: int):
        for channel in range(count):
            self.channel_allocations[channel] = {
                "cks": [],
                "ckr": []
            }

        required_ports = {
            "cks": [],
            "ckr": []
        }

        for key in (
                ("cks", "data"),
                ("cks", "control"),
                ("ckr", "data"),
                ("ckr", "control")
        ):
            group = self.create_group(key)
            hw_ports = [m for m in group.hw_mapping() if m != INVALID_HARDWARE_PORT]
            kernel = key[0]
            required_ports[kernel] += [(key[1], hw_port) for hw_port in hw_ports]

        for kernel in required_ports:
            ports = required_ports[kernel]
            for channel in range(count):
                self.channel_allocations[channel][kernel] = round_robin(ports, channel, count)


class FPGA:
    def __init__(self, node: str, name: str, program: Program):
        self.node = node
        self.name = name
        self.channels: List[Channel] = [Channel(self, i) for i in range(CHANNELS_PER_FPGA)]
        self.rank = None
        self.program = program

    def has_channel(self, index: int):
        return self.channels[index] is not None

    def add_channel(self, index):
        assert not self.has_channel(index)
        self.channels[index] = Channel(self, index)

    def key(self):
        return "{}:{}".format(self.node, self.name)

    def __repr__(self):
        return "FPGA({})".format(self.key())


class ProgramMapping:
    def __init__(self, programs: List[Program], fpga_map: Dict[str, Program]):
        """
        :param fpga_map: mapping from FPGA id to a program
        """
        self.programs = programs
        self.fpga_map = fpga_map


def are_ops_consecutive(ops: List[SmiOperation]) -> bool:
    if not ops:
        return True

    ports = set([op.logical_port for op in ops])
    start = min(ports)
    end = max(ports)
    return sorted(ports) == list(range(start, end + 1))


def target_index(source: int, target: int) -> int:
    """Returns the index of the target channel, skipping the index of the self channel.
    Used in inter CKS/CKR communication"""
    assert source != target
    if target < source:
        return target
    return target - 1


def map_ports(ports: List[SmiOperation], key_fn, offset: int = 0) -> List[int]:
    hardware_ports = []
    for port in ports:
        allocated_ports = key_fn(port)
        hw_port = INVALID_HARDWARE_PORT
        if allocated_ports:
            hw_port = offset
            offset += allocated_ports

        hardware_ports.append(hw_port)

    return hardware_ports


def count_allocations(ports: List[SmiOperation], key_fn) -> int:
    return sum(key_fn(p) for p in ports)

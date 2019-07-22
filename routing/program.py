from typing import List, Dict

from ops import SmiOperation, KEY_CKS_DATA, KEY_CKS_CONTROL, KEY_CKR_DATA, KEY_CKR_CONTROL, \
    OP_MAPPING
from utils import round_robin

COST_INTER_FPGA = 100
COST_INTRA_FPGA = 1
CHANNELS_PER_FPGA = 4


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
        for i in range(len(self.fpga.channels)):
            if i != self.index:
                yield i

    def __repr__(self):
        return "Channel({}:{})".format(self.fpga, self.index)


def validate_allocations(channel_allocations):
    ch_to_port = {}
    for allocations in channel_allocations.values():
        for (op, ch) in allocations:
            ports = ch_to_port.setdefault(ch, set())
            if op.logical_port in ports:
                raise FailedAllocation(op)
            ports.add(op.logical_port)


def allocate_channels(operations: List[SmiOperation], p2p_rendezvous: bool, count: int):
    channel_allocations = {}

    for channel in range(count):
        channel_allocations[channel] = []

    required_channels = (
        KEY_CKS_DATA,
        KEY_CKS_CONTROL,
        KEY_CKR_DATA,
        KEY_CKR_CONTROL
    )

    op_channels = {
        "cks": [],
        "ckr": []
    }

    for chan in required_channels:
        for op in operations:
            usage = op.channel_usage(p2p_rendezvous)
            if chan in usage:
                op_channels[chan.split("_")[0]].append((op, chan))

    for (key, ops) in op_channels.items():
        for channel in range(count):
            channel_allocations[channel] += round_robin(ops, channel, count)
    return channel_allocations


class Program:
    """
    Contains all information necessary to generate code for a single rank.

    Program has to provide:

    Globally:
    - number of logical ports
    For each channel:
    - allocated (operation, channel)
    For a logical port and channel key:
    - allocated channel
    """
    def __init__(self,
                 operations: List[SmiOperation],
                 consecutive_read_limit=8,
                 max_ranks=8,
                 p2p_rendezvous=True,
                 channel_count=CHANNELS_PER_FPGA):

        self.consecutive_read_limit = consecutive_read_limit
        self.max_ranks = max_ranks
        self.p2p_rendezvous = p2p_rendezvous
        self.operations = sorted(operations, key=lambda op: op.logical_port)
        self.channel_count = channel_count

        self.logical_port_count = max((op.logical_port for op in operations), default=0) + 1
        self.channel_allocations = allocate_channels(self.operations, p2p_rendezvous, channel_count)
        validate_allocations(self.channel_allocations)

    def get_channel_allocations(self, channel: int):
        return self.channel_allocations[channel]

    def get_channel_allocations_with_prefix(self, channel: int, prefix: str):
        allocations = self.get_channel_allocations(channel)
        return tuple((op, key) for (op, key) in allocations if key.startswith(prefix))

    def get_channel_for_port_key(self, logical_port: int, key: str):
        for (channel, allocations) in self.channel_allocations.items():
            for (op, ch) in allocations:
                if op.logical_port == logical_port and ch == key:
                    return channel
        return None

    def get_ops_by_type(self, type: str) -> List[SmiOperation]:
        cls = OP_MAPPING[type]
        return [op for op in self.operations if isinstance(op, cls)]


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


def target_index(source: int, target: int) -> int:
    """Returns the index of the target channel, skipping the index of the self channel.
    Used in inter CKS/CKR communication"""
    assert source != target
    if target < source:
        return target
    return target - 1

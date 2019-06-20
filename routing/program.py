from typing import List, Dict

from utils import round_robin

COST_INTER_FPGA = 100
COST_INTRA_FPGA = 1
CHANNELS_PER_FPGA = 4

INVALID_HARDWARE_PORT = -1


class PortType:
    def __init__(self, id):
        # TODO: check if id is needed
        self.id = id

    def cks_data_ports(self) -> int:
        return 0

    def cks_control_ports(self) -> int:
        return 0

    def ckr_data_ports(self) -> int:
        return 0

    def ckr_control_ports(self) -> int:
        return 0

    def cks_hw_port_count(self) -> int:
        return sum((
            self.cks_data_ports(),
            self.cks_control_ports()
        ))

    def ckr_hw_port_count(self) -> int:
        return sum((
            self.ckr_data_ports(),
            self.ckr_control_ports()
        ))

    def hw_port_count(self) -> int:
        return sum((
            self.cks_data_ports(),
            self.cks_control_ports(),
            self.ckr_data_ports(),
            self.ckr_control_ports()
        ))


class Push(PortType):
    def cks_data_ports(self) -> int:
        return 1

    def ckr_control_ports(self) -> int:
        return 1


class Pop(PortType):
    def ckr_data_ports(self) -> int:
        return 1

    def cks_control_ports(self) -> int:
        return 1


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
    """
    def __init__(self, buffer_size: int, ports: List[PortType]):
        self.buffer_size = buffer_size
        self.ports = ports

    def logical_port_count(self) -> int:
        return len(self.ports)

    def hw_port_count(self) -> int:
        return sum([p.hw_port_count() for p in self.ports])

    def cks_hw_port_count(self) -> int:
        return sum([p.cks_hw_port_count() for p in self.ports])

    def ckr_hw_port_count(self) -> int:
        return sum([p.ckr_hw_port_count() for p in self.ports])

    def cks_data_mapping(self) -> List[int]:
        """
        Maps logical ports to hardware ports, for cks data ports.
        If the port mapping should not be used, -1 is returned in its place.
        """
        return map_ports(self.ports, lambda p: p.cks_data_ports())

    def cks_control_mapping(self) -> List[int]:
        return map_ports(self.ports, lambda p: p.cks_control_ports(),
                         count_allocations(self.ports, lambda p: p.cks_data_ports()))

    def ckr_data_mapping(self) -> List[int]:
        return map_ports(self.ports, lambda p: p.ckr_data_ports())

    def ckr_control_mapping(self) -> List[int]:
        return map_ports(self.ports, lambda p: p.ckr_control_ports(),
                         count_allocations(self.ports, lambda p: p.ckr_data_ports()))

    def cks_hw_ports(self, channel: Channel, channel_count) -> List[int]:
        return round_robin(list(range(self.cks_hw_port_count())), channel.index, channel_count)

    def ckr_hw_ports(self, channel: Channel, channel_count) -> List[int]:
        return round_robin(list(range(self.ckr_hw_port_count())), channel.index, channel_count)

    def get_channel_for_hw_port(self, hw_port, channel_count):
        return hw_port % channel_count


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


def map_ports(ports: List[PortType], key_fn, offset: int = 0) -> List[int]:
    hardware_ports = []
    for port in ports:
        allocated_ports = key_fn(port)
        hw_port = INVALID_HARDWARE_PORT
        if allocated_ports:
            hw_port = offset
            offset += allocated_ports

        hardware_ports.append(hw_port)

    return hardware_ports


def count_allocations(ports: List[PortType], key_fn) -> int:
    return sum(key_fn(p) for p in ports)

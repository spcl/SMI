from typing import List, Dict


class PortInfo:
    """
    Contains ID and type of a port.
    """
    def __init__(self, id: int, type: str):
        self.id = id
        self.type = type


class Program:
    """
    Contains all information necessary to generate code for a single rank.
    """
    def __init__(self, buffer_size: int, ports: List[PortInfo]):
        self.buffer_size = buffer_size
        self.ports = ports


class ProgramMapping:
    def __init__(self, programs: List[Program], fpga_map: Dict[str, Program]):
        """
        :param fpga_map: mapping from FPGA id to a program
        """
        self.programs = programs
        self.fpga_map = fpga_map

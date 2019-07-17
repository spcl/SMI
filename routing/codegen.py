import logging
import os
from typing import List

import jinja2
from networkx import Graph

from program import Channel, target_index, FPGA, Program


def read_template_file(path):
    templates = os.path.join(os.path.dirname(__file__), "templates")
    loader = jinja2.FileSystemLoader(searchpath=templates)

    logging.basicConfig()
    logger = logging.getLogger('logger')
    logger = jinja2.make_logging_undefined(logger=logger, base=jinja2.Undefined)

    env = jinja2.Environment(loader=loader, undefined=logger)
    env.lstrip_blocks = True
    env.trim_blocks = True
    return env.get_template(path)


def channel_name(src: Channel, out: bool, graph: Graph) -> str:
    remote_channel = None
    for (_, to) in graph.edges(src):
        if to.fpga != src.fpga:
            remote_channel = to

    if remote_channel:
        remote_channel = "r{}c{}".format(remote_channel.fpga.rank, remote_channel.index)
    else:
        remote_channel = "unconnected"

    local_channel = "r{}c{}".format(src.fpga.rank, src.index)
    if not out:
        tmp = local_channel
        local_channel = remote_channel
        remote_channel = tmp

    return "{}_{}".format(local_channel, remote_channel)


def generate_program_host(program: Program) -> str:
    template = read_template_file("host.cl")
    return template.render(program=program)


def generate_program_device(fpga: FPGA, fpgas: List[FPGA], graph: Graph, channels_per_fpga: int) -> str:
    template = read_template_file("device.cl")
    return template.render(channels=fpga.channels,
                           channels_per_fpga=channels_per_fpga,
                           target_index=target_index,
                           program=fpga.program,
                           fpgas=fpgas,
                           channel_name=lambda channel, out: channel_name(channel, out, graph))

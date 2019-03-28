import logging
import os
from typing import List

import jinja2

from common import Channel, target_index, FPGA


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


def generate_kernels(fpgas: List[FPGA], channels: List[Channel], channels_per_fpga: int, tag_count: int) -> str:
    template = read_template_file("routing.cl")

    return template.render(channels=channels,
                           channels_per_fpga=channels_per_fpga,
                           target_index=target_index,
                           rank_count=len(fpgas),
                           tag_count=tag_count)

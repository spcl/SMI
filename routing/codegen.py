import math
import os
from typing import List

import jinja2

from common import Channel, target_index


def read_template_file(path):
    templates = os.path.join(os.path.dirname(__file__), "templates")
    loader = jinja2.FileSystemLoader(searchpath=templates)
    env = jinja2.Environment(loader=loader)
    env.lstrip_blocks = True
    env.trim_blocks = True
    return env.get_template(path)


def generate_kernels(channels: List[Channel], channels_per_fpga: int, tag_count: int) -> str:
    template = read_template_file("routing.cl")

    return template.render(channels=channels,
                           tag_count=tag_count,
                           channels_per_fpga=channels_per_fpga,
                           tags_per_channel=math.ceil(tag_count / channels_per_fpga),
                           target_index=target_index)

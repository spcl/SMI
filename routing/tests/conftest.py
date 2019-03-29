import os
import sys
from typing import Union

from io import StringIO

sys.path.append(os.path.abspath(os.path.dirname(os.path.dirname(__file__))))

from common import Channel
from routing import create_routing_context


def init_graph(input):
    stream = StringIO(input)
    return create_routing_context(stream)


def get_channel(graph, key, index) -> Union[Channel, None]:
    for channel in graph.nodes:
        if channel.fpga.key() == key and channel.index == index:
            return channel
    return None

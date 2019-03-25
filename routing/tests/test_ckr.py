from io import StringIO
from typing import Union

import pytest

from common import Channel
from routing import create_routing_context
from table import ckr_routing_table, NoRouteFound


def init_graph(input):
    stream = StringIO(input)
    return create_routing_context(stream)


def get_channel(graph, key, index) -> Union[Channel, None]:
    for channel in graph.nodes:
        if channel.fpga.key() == key and channel.index == index:
            return channel
    return None


def test_ckr_table():
    ctx = init_graph("""
    N0:F0:ch0 <-> N0:F1:ch0
    N1:F0:ch0 <-> N0:F0:ch1
""")

    graph, routes, fpgas = (ctx.graph, ctx.routes, ctx.fpgas)

    a = get_channel(graph, "N0:F0", 0)
    assert ckr_routing_table(routes, fpgas, a) == [1, 0, 2]

    b = get_channel(graph, "N0:F0", 1)
    assert ckr_routing_table(routes, fpgas, b) == [1, 4, 0]

    c = get_channel(graph, "N0:F1", 0)
    assert ckr_routing_table(routes, fpgas, c) == [0, 1, 0]

    d = get_channel(graph, "N1:F0", 0)
    assert ckr_routing_table(routes, fpgas, d) == [0, 0, 1]


def test_ckr_no_route():
    ctx = init_graph("""
    N0:F0:ch0 <-> N0:F1:ch0
    N1:F0:ch0 <-> N1:F2:ch1
""")
    graph, routes, fpgas = (ctx.graph, ctx.routes, ctx.fpgas)
    ch = get_channel(graph, "N0:F0", 0)
    with pytest.raises(NoRouteFound):
        ckr_routing_table(routes, fpgas, ch)

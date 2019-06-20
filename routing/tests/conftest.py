import os
import shutil
import sys
from io import StringIO
from typing import Union

import pytest
from networkx import Graph

from common import RoutingContext
from parser import parse_fpga_connections, parse_programs

sys.path.append(os.path.abspath(os.path.dirname(os.path.dirname(__file__))))

from program import Channel
from routing import create_routing_context

PYTEST_DIR = os.path.dirname(__file__)
WORK_DIR = os.path.join(PYTEST_DIR, "work")
DATA_DIR = os.path.join(PYTEST_DIR, "data")


def prepare():
    """Prepare working directory
    If directory exists then it is cleaned;
    If it does not exists then it is created.
    """
    if os.path.isdir(WORK_DIR):
        for root, dirs, files in os.walk(WORK_DIR):
            for d in dirs:
                os.chmod(os.path.join(root, d), 0o700)
            for f in files:
                os.chmod(os.path.join(root, f), 0o700)
        for item in os.listdir(WORK_DIR):
            path = os.path.join(WORK_DIR, item)
            if os.path.isfile(path):
                os.unlink(path)
            else:
                shutil.rmtree(path)
    else:
        os.makedirs(WORK_DIR)
    os.chdir(WORK_DIR)


def get_routing_ctx(programs: str, connections: str) -> RoutingContext:
    connections = parse_fpga_connections(connections)
    programs = parse_programs(programs)
    return create_routing_context(connections, programs)


def get_channel(graph: Graph, key: str, index: int) -> Union[Channel, None]:
    for channel in graph.nodes:
        if channel.fpga.key() == key and channel.index == index:
            return channel
    return None


def get_data(path: str) -> str:
    return os.path.join(DATA_DIR, path)


class FileTester:
    def check(self, path: str, content: str):
        file_path = get_data(path)
        with open(file_path) as f:
            file_content = f.read()

        ok = False
        try:
            assert file_content == content
            ok = True
        finally:
            if not ok:
                with open("{}.fail".format(os.path.basename(path)), "w") as f:
                    f.write(content)


@pytest.yield_fixture(autouse=True, scope="function")
def file_tester():
    prepare()
    yield FileTester()

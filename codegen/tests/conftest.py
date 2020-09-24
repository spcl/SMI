import json
import os
import shutil
import subprocess
import sys
from typing import Union, List

import pytest
from networkx import Graph

from common import RoutingContext
from ops import SmiOperation
from serialization import parse_smi_operation

sys.path.append(os.path.abspath(os.path.dirname(os.path.dirname(__file__))))

from program import Channel, ProgramMapping, Program, FPGA
from routing import create_routing_context

PYTEST_DIR = os.path.dirname(__file__)
WORK_DIR = os.path.join(PYTEST_DIR, "work")
DATA_DIR = os.path.join(PYTEST_DIR, "data")

ROOT_DIR = os.path.dirname(os.path.dirname(PYTEST_DIR))
REWRITER_BUILD_DIR = os.environ.get("REWRITER_DIR", ROOT_DIR.join("build/rewriter"))


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


def get_routing_ctx(program: Program, connections) -> RoutingContext:
    fpgas = tuple(fpga for (fpga, _) in connections.keys()) + tuple(fpga for (fpga, _) in connections.values())
    fpga_map = {
        fpga: program for fpga in fpgas
    }
    for (k, v) in dict(connections).items():
        connections[v] = k

    mapping = ProgramMapping([program], fpga_map)
    return create_routing_context(connections, mapping)


def get_channel(graph: Graph, key: str, index: int) -> Union[Channel, None]:
    for channel in graph.nodes:
        if channel.fpga.key() == key and channel.index == index:
            return channel
    return None


def get_fpga(fpgas: List[FPGA], key: str) -> Union[FPGA, None]:
    for fpga in fpgas:
        if fpga.key() == key:
            return fpga
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


class RewriteTester:
    def check(self, path: str, operations: List[SmiOperation]):
        orig_file = get_data("{}.cl".format(path))
        work_file = os.path.join(WORK_DIR, "{}.cl".format(path))
        expected_file = get_data("{}-expected.cl".format(path))

        shutil.copyfile(orig_file, work_file)
        ok = False

        try:
            result = subprocess.run([
                os.path.join(REWRITER_BUILD_DIR, "rewriter"),
                "-extra-arg=-I{}".format(os.path.join(ROOT_DIR, "include")),
                work_file
            ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout = result.stdout.decode()
            parsed_ops = [parse_smi_operation(json.loads(line)) for line in stdout.splitlines() if line]
            assert parsed_ops == operations

            with open(expected_file) as expected:
                with open(work_file) as work:
                    assert work.read() == expected.read()

            ok = True
        finally:
            if ok:
                shutil.rmtree(WORK_DIR, ignore_errors=True)
            else:
                print(result.stderr.decode())


@pytest.yield_fixture(autouse=True, scope="function")
def rewrite_tester():
    prepare()
    yield RewriteTester()

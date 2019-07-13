import os
import shutil
import subprocess
from typing import List

import click

from codegen import generate_program_device, generate_program_host
from common import write_nodefile
from ops import Push, Pop, Broadcast, Reduce
from parser import parse_fpga_connections
from program import Channel, CHANNELS_PER_FPGA, Program, ProgramMapping
from routing import create_routing_context
from routing_table import serialize_to_array, cks_routing_table, ckr_routing_table


def prepare_directory(path):
    if not os.path.exists(path):
        os.makedirs(path, exist_ok=True)


def write_table(channel: Channel, prefix: str, table: List[int], output_folder):
    bytes = serialize_to_array(table)
    filename = "{}-rank{}-channel{}".format(prefix, channel.fpga.rank, channel.index)

    with open(os.path.join(output_folder, filename), "wb") as f:
        f.write(bytes)


def copy_files(src_dir, dest_dir, files):
    """
    Copies device source files from the source directory to the output directory.
    Returns a list of tuples (src, dest) path.
    """
    for file in files:
        src_path = os.path.join(src_dir, file)
        dest_path = os.path.join(dest_dir, file)
        dest_dir = os.path.dirname(dest_path)
        os.makedirs(dest_dir, exist_ok=True)
        shutil.copyfile(src_path, dest_path, follow_symlinks=True)
        yield (src_path, dest_path)


def parse_op(line):
    items = line.split(" ")
    mapping = {
        "push": Push,
        "pop": Pop,
        "broadcast": Broadcast,
        "reduce": Reduce
    }
    port = int(items[1])
    return mapping[items[0]](port, *items[2:])


def rewrite(rewriter, file, include_dirs):
    args = [rewriter, file]
    for dir in include_dirs:
        args += ["-extra-arg=-I{}".format(dir)]

    process = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output = process.stdout.decode()

    ops = []
    for line in output.splitlines():
        if line:
            ops.append(parse_op(line.strip()))

    return ops


@click.command()
@click.argument("connections")
@click.argument("rewriter")
@click.argument("src-dir")
@click.argument("dest-dir")
@click.argument("host-src")
@click.argument("device-src")
@click.argument("device-input", nargs=-1)
@click.option("--include")
def build(connections, rewriter, src_dir, dest_dir, host_src, device_src, device_input, include):
    """
    Transpiles device code and generates routing tables.
    :param connections: path to a file with FPGA connections
    :param rewriter: path to the rewriter
    :param src_dir: root directory of device source files
    :param dest_dir: root directory of generated device source files
    :param host_src: path to generated host source file
    :param device_src: path to generated device source file
    :param device_input: list of device sources
    :param include: list of include directories for device sources
    """
    paths = list(copy_files(src_dir, dest_dir, device_input))

    ops = []
    include_dirs = set(include.split(" "))
    for (src, dest) in paths:
        ops += rewrite(rewriter, dest, include_dirs)

    ops = sorted(ops, key=lambda op: op.logical_port)
    program = Program(ops)

    with open(connections) as connection_file:
        connections = parse_fpga_connections(connection_file.read())
        program_mapping = ProgramMapping([program], {
            fpga: program for fpga in set(fpga for (fpga, _) in connections.keys())
        })
        ctx = create_routing_context(connections, program_mapping)

    for fpga in ctx.fpgas:
        for channel in fpga.channels:
            cks_table = cks_routing_table(ctx.routes, ctx.fpgas, channel)
            write_table(channel, "cks", cks_table, dest_dir)
            ckr_table = ckr_routing_table(channel, CHANNELS_PER_FPGA, fpga.program)
            write_table(channel, "ckr", ckr_table, dest_dir)

    fpgas = ctx.fpgas
    if fpgas:
        with open(device_src, "w") as f:
            f.write(generate_program_device(fpgas[0], fpgas, ctx.graph, CHANNELS_PER_FPGA))
        with open(host_src, "w") as f:
            f.write(generate_program_host(fpgas[0]))

    with open(os.path.join(dest_dir, "hostfile"), "w") as f:
        write_nodefile(ctx.fpgas, f)


@click.group()
def cli():
    pass


if __name__ == "__main__":
    cli.add_command(build)
    cli()

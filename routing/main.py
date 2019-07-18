import os
from typing import List

import click

from codegen import generate_program_device, generate_program_host
from common import write_nodefile
from program import Channel, CHANNELS_PER_FPGA, Program, ProgramMapping
from rewrite import copy_files, rewrite
from routing import create_routing_context
from routing_table import serialize_to_array, cks_routing_table, ckr_routing_table
from serialization import serialize_program, parse_routing_file, parse_program


def prepare_directory(path):
    if not os.path.exists(path):
        os.makedirs(path, exist_ok=True)


def write_table(channel: Channel, prefix: str, table: List[int], output_folder):
    bytes = serialize_to_array(table)
    filename = "{}-rank{}-channel{}".format(prefix, channel.fpga.rank, channel.index)

    with open(os.path.join(output_folder, filename), "wb") as f:
        f.write(bytes)


@click.command()
@click.argument("routing_file")
@click.argument("rewriter")
@click.argument("src-dir")
@click.argument("dest-dir")
@click.argument("device-src")
@click.argument("output-program")
@click.argument("device-input", nargs=-1)
@click.option("--include")
def codegen_device(routing_file, rewriter, src_dir, dest_dir, device_src, output_program, device_input, include):
    """
    Transpiles device code and generates device kernels and host initialization code.
    :param routing_file: path to a file with FPGA connections and FPGA-to-program mapping
    :param rewriter: path to the rewriter
    :param src_dir: root directory of device source files
    :param dest_dir: root directory of generated device source files
    :param device_src: path to generated device source file
    :param output_program: path to generated JSON program description
    :param device_input: list of device sources
    :param include: list of include directories for device sources
    """
    paths = list(copy_files(src_dir, dest_dir, device_input))

    with open("rewrite.log", "w") as f:
        ops = []
        include_dirs = set(include.split(" "))
        for (src, dest) in paths:
            ops += rewrite(rewriter, dest, include_dirs, f)

    ops = sorted(ops, key=lambda op: op.logical_port)
    program = Program(ops)

    with open(routing_file) as rf:
        (connections, mapping) = parse_routing_file(rf.read(), ignore_programs=True)
        program_mapping = ProgramMapping([program], {
            fpga: program for fpga in set(fpga for (fpga, _) in connections.keys())
        })
        ctx = create_routing_context(connections, program_mapping)

    fpgas = ctx.fpgas
    if fpgas:
        with open(device_src, "w") as f:
            f.write(generate_program_device(fpgas[0], fpgas, ctx.graph, CHANNELS_PER_FPGA))

    with open(output_program, "w") as f:
        f.write(serialize_program(program))


@click.command()
@click.argument("host-src")
@click.argument("program-metadata", nargs=-1)
def codegen_host(host_src, program_metadata):
    """
    Creates routing tables and hostfile.
    :param host_src: path to a file with generated host code
    :param program_metadata: list of program metadata files
    """
    programs = []
    for program in program_metadata:
        with open(program) as pf:
            programs.append(parse_program(pf.read()))

    with open(host_src, "w") as f:
        f.write(generate_program_host(programs))


@click.command()
@click.argument("routing_file")
@click.argument("dest_dir")
def route(routing_file, dest_dir):
    """
    Creates routing tables and hostfile.
    :param routing_file: path to a file with FPGA connections and FPGA-to-program mapping
    :param dest_dir: path to a directory where routing tables and the hostfile will be generated
    """
    with open(routing_file) as rf:
        (connections, mapping) = parse_routing_file(rf.read())
        ctx = create_routing_context(connections, mapping)

    for fpga in ctx.fpgas:
        for channel in fpga.channels:
            cks_table = cks_routing_table(ctx.routes, ctx.fpgas, channel)
            write_table(channel, "cks", cks_table, dest_dir)
            ckr_table = ckr_routing_table(channel, CHANNELS_PER_FPGA, fpga.program)
            write_table(channel, "ckr", ckr_table, dest_dir)

    with open(os.path.join(dest_dir, "hostfile"), "w") as f:
        write_nodefile(ctx.fpgas, f)


@click.group()
def cli():
    pass


if __name__ == "__main__":
    cli.add_command(codegen_device)
    cli.add_command(codegen_host)
    cli.add_command(route)
    cli()

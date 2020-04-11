import os

import click

from codegen import generate_program_device, generate_program_host
from common import write_nodefile
from program import CHANNELS_PER_FPGA, Channel, Program, ProgramMapping
from rewrite import copy_files, rewrite
from routing import create_routing_context
from routing_table import RoutingTable, ckr_routing_table, cks_routing_tables
from serialization import parse_program, parse_routing_file, serialize_program


def prepare_directory(path):
    if not os.path.exists(path):
        os.makedirs(path, exist_ok=True)


def write_file(path, content, binary=False):
    prepare_directory(os.path.dirname(os.path.abspath(path)))
    with open(path, "wb" if binary else "w") as f:
        f.write(content)


def write_table(channel: Channel, prefix: str, table: RoutingTable, output_folder):
    bytes = table.serialize()
    filename = "{}-rank{}-channel{}".format(prefix, channel.fpga.rank, channel.index)

    write_file(os.path.join(output_folder, filename), bytes, binary=True)


@click.command()
@click.argument("routing_file")
@click.argument("rewriter")
@click.argument("src-dir")
@click.argument("dest-dir")
@click.argument("device-src")
@click.argument("output-program")
@click.argument("device-input", nargs=-1)
@click.option("--include")
@click.option("--consecutive-read-limit", default=8)
@click.option("--max-ranks", default=8)
@click.option("--p2p-rendezvous", default=True)
def codegen_device(routing_file, rewriter, src_dir, dest_dir, device_src,
                   output_program, device_input,
                   include, consecutive_read_limit, max_ranks, p2p_rendezvous):
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
    :param consecutive_read_limit: how many reads should be performed in succession from a single channel in CKR/CKS
    :param max_ranks: maximum number of ranks in the cluster
    :param p2p_rendezvous: whether to use rendezvous for P2P operations
    """
    paths = list(copy_files(src_dir, dest_dir, device_input))

    p2p_rendezvous = True if p2p_rendezvous in (True, 1, "1", "ON") else False

    with open("rewrite.log", "w") as f:
        ops = []
        include_dirs = set(include.split(" "))
        for (src, dest) in paths:
            ops += rewrite(rewriter, dest, include_dirs, f)

    ops = sorted(ops, key=lambda op: op.logical_port)
    program = Program(ops, consecutive_read_limit, max_ranks, p2p_rendezvous)

    with open(routing_file) as rf:
        (connections, mapping) = parse_routing_file(rf.read(), ignore_programs=True)
        program_mapping = ProgramMapping([program], {
            fpga: program for fpga in set(fpga for (fpga, _) in connections.keys())
        })
        ctx = create_routing_context(connections, program_mapping)

    fpgas = ctx.fpgas
    if fpgas:
        write_file(device_src,
                   generate_program_device(fpgas[0], fpgas, ctx.graph, CHANNELS_PER_FPGA))

    write_file(output_program, serialize_program(program))


@click.command()
@click.argument("host-src")
@click.argument("metadata", nargs=-1)
def codegen_host(host_src, metadata):
    """
    Creates routing tables and hostfile.
    :param host_src: path to a file with generated host code
    :param metadata: list of program metadata files
    """
    programs = []
    for program in metadata:
        with open(program) as pf:
            basename = os.path.splitext(os.path.basename(program))[0]
            programs.append((basename, parse_program(pf.read())))

    write_file(host_src, generate_program_host(programs))


@click.command()
@click.argument("routing_file")
@click.argument("dest_dir")
@click.argument("metadata", nargs=-1)
def route(routing_file, dest_dir, metadata):
    """
    Creates routing tables and hostfile.
    :param routing_file: path to a file with FPGA connections and FPGA-to-program mapping
    :param dest_dir: path to a directory where routing tables and the hostfile will be generated
    :param metadata: list of program metadata files
    """
    prepare_directory(os.path.abspath(dest_dir))

    with open(routing_file) as rf:
        (connections, mapping) = parse_routing_file(rf.read(), metadata)
        ctx = create_routing_context(connections, mapping)

    for fpga in ctx.fpgas:
        for channel in fpga.channels:
            ckr_table = ckr_routing_table(channel, CHANNELS_PER_FPGA, fpga.program)
            write_table(channel, "ckr", ckr_table, dest_dir)
        tables = cks_routing_tables(fpga, ctx.fpgas, ctx.routes)
        for (channel, table) in tables.items():
            write_table(channel, "cks", table, dest_dir)

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

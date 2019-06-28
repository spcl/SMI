import os
from typing import List

import click

from codegen import generate_program_device, generate_program_host
from common import write_nodefile
from parser import parse_programs, parse_fpga_connections
from program import Channel, CHANNELS_PER_FPGA
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


@click.command()
@click.argument("program-description")
@click.argument("connection-list")
@click.argument("output-folder")
def build(program_description, connection_list, output_folder):
    with open(program_description) as program_file:
        with open(connection_list) as connection_file:
            program_mapping = parse_programs(program_file.read())
            connections = parse_fpga_connections(connection_file.read())
            ctx = create_routing_context(connections, program_mapping)

    prepare_directory(output_folder)

    for fpga in ctx.fpgas:
        for channel in fpga.channels:
            cks_table = cks_routing_table(ctx.routes, ctx.fpgas, channel)
            write_table(channel, "cks", cks_table, output_folder)
            ckr_table = ckr_routing_table(channel, CHANNELS_PER_FPGA, fpga.program)
            write_table(channel, "ckr", ckr_table, output_folder)

    for (index, program) in enumerate(program_mapping.programs):
        fpgas = [fpga for fpga in ctx.fpgas if fpga.program is program]
        if fpgas:
            with open(os.path.join(output_folder, "smi-device-{}.h".format(index)), "w") as f:
                f.write(generate_program_device(fpgas[0], fpgas, ctx.graph, CHANNELS_PER_FPGA))
            with open(os.path.join(output_folder, "smi-host-{}.h".format(index)), "w") as f:
                f.write(generate_program_host(fpgas[0]))

    with open(os.path.join(output_folder, "hostfile"), "w") as f:
        write_nodefile(ctx.fpgas, f)


@click.group()
def cli():
    pass


if __name__ == "__main__":
    cli.add_command(build)
    cli()

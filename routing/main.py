import os
from typing import List

import click

from common import Channel, write_nodefile
from routing import create_routing_context
from table import ckr_routing_table, serialize_to_array, cks_routing_table


def prepare_directory(path):
    if not os.path.exists(path):
        os.makedirs(path, exist_ok=True)


def write_table(channel: Channel, prefix: str, table: List[int], output_folder):
    bytes = serialize_to_array(table)
    filename = "{}-rank{}-channel{}".format(prefix, channel.fpga.rank, channel.index)

    with open(os.path.join(output_folder, filename), "wb") as f:
        f.write(bytes)


@click.command()
@click.argument("nodelist")
@click.argument("output-folder")
def build(nodelist, output_folder):
    with open(nodelist) as f:
        ctx = create_routing_context(f)

    prepare_directory(output_folder)

    print("Number of FPGAs: {}".format(len(ctx.fpgas)))
    print("Number of active channels: {}".format(len(ctx.graph.nodes)))

    for channel in ctx.graph.nodes:
        write_table(channel, "ckr", ckr_routing_table(ctx.routes, ctx.fpgas, channel), output_folder)
        write_table(channel, "cks", cks_routing_table(ctx.routes, ctx.fpgas, channel), output_folder)

    with open(os.path.join(output_folder, "hostfile"), "w") as f:
        write_nodefile(ctx.fpgas, f)


@click.group()
def cli():
    pass


if __name__ == "__main__":
    cli.add_command(build)
    cli()

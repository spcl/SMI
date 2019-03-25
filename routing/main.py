import os
from typing import List

import click

from common import Channel, write_nodefile, CHANNELS_PER_FPGA
from routing import create_routing_context
from table import serialize_to_array, cks_routing_table, ckr_routing_table


def prepare_directory(path):
    if not os.path.exists(path):
        os.makedirs(path, exist_ok=True)


def write_table(channel: Channel, prefix: str, table: List[int], output_folder):
    bytes = serialize_to_array(table)
    filename = "{}-rank{}-channel{}".format(prefix, channel.fpga.rank, channel.index)

    with open(os.path.join(output_folder, filename), "wb") as f:
        f.write(bytes)


@click.command()
@click.argument("connection-list")
@click.argument("output-folder")
@click.argument("tag-count", default=1)
def build(connection_list, output_folder, tag_count):
    with open(connection_list) as f:
        ctx = create_routing_context(f)

    prepare_directory(output_folder)

    print("Number of FPGAs: {}".format(len(ctx.fpgas)))
    print("Number of active channels: {}".format(len(ctx.graph.nodes)))

    for channel in sorted(ctx.graph.nodes, key=lambda c: "{}:{}".format(c.fpga.key(), c.index)):
        print(channel)

        cks_table = cks_routing_table(ctx.routes, ctx.fpgas, channel)
        write_table(channel, "cks", cks_table, output_folder)
        print("CKS: {}".format(cks_table))

        ckr_table = ckr_routing_table(channel, CHANNELS_PER_FPGA, tag_count)
        write_table(channel, "ckr", ckr_table, output_folder)
        print("CKR: {}\n".format(ckr_table))

    with open(os.path.join(output_folder, "hostfile"), "w") as f:
        write_nodefile(ctx.fpgas, f)


@click.group()
def cli():
    pass


if __name__ == "__main__":
    cli.add_command(build)
    cli()

import click

from routing import create_routing_context
from table import ckr_routing_table, serialize_to_array


@click.command()
@click.argument("nodelist")
def ckr_table(nodelist):
    with open(nodelist) as f:
        ctx = create_routing_context(f)

    print("Number of ranks: {}".format(len(ctx.ranks)))
    print("Number of active channels: {}".format(len(ctx.graph.nodes)))

    for channel in ctx.graph.nodes:
        table = ckr_routing_table(ctx.routes, ctx.ranks, channel)
        print(serialize_to_array(table))


@click.group()
def cli():
    pass


if __name__ == "__main__":
    cli.add_command(ckr_table)
    cli()

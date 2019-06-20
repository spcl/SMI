from codegen import generate_program
from ops import Push, Pop
from parser import parse_fpga_connections
from program import Program, ProgramMapping
from routing import create_routing_context


def test_codegen(file_tester):
    program = Program(4096, [
        Push(0),
        Pop(1),
        Push(2),
        Pop(3),
        Pop(4)
    ])

    mapping = ProgramMapping([program], {
        "n1:f1": program,
        "n1:f2": program,
        "n2:f1": program,
        "n3:f1": program
    })

    connections = parse_fpga_connections("""
n1:f1:ch0 <-> n1:f2:ch0
n1:f2:ch1 <-> n2:f1:ch1
n1:f2:ch2 <-> n3:f1:ch1
n2:f1:ch0 <-> n1:f1:ch1
""")

    ctx = create_routing_context(connections, mapping)

    file_tester.check("smi-1.h", generate_program(ctx.fpgas[0], ctx.fpgas, ctx.graph, 4))

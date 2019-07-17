from codegen import generate_program_device, generate_program_host
from ops import Push, Pop, Broadcast, Reduce, Scatter, Gather
from serialization import parse_routing_file
from program import Program, ProgramMapping
from routing import create_routing_context


def test_codegen_device(file_tester):
    program = Program([
        Push(0),
        Pop(0),
        Push(1),
        Pop(2),
        Broadcast(3),
        Broadcast(4),
        Push(5),
        Reduce(6, "float", "add"),
        Scatter(7, "double"),
        Gather(8, "char")
    ])

    mapping = ProgramMapping([program], {
        "n1:f1": program,
        "n1:f2": program,
        "n2:f1": program,
        "n3:f1": program
    })

    connections = parse_routing_file("""
n1:f1:ch0 <-> n1:f2:ch0
n1:f2:ch1 <-> n2:f1:ch1
n1:f2:ch2 <-> n3:f1:ch1
n2:f1:ch0 <-> n1:f1:ch1
""")

    ctx = create_routing_context(connections, mapping)

    file_tester.check("smi-device-1.h", generate_program_device(ctx.fpgas[0], ctx.fpgas, ctx.graph, 4))


def test_codegen_host(file_tester):
    program = Program([
        Push(0),
        Pop(0),
        Push(1),
        Pop(2),
        Broadcast(3),
        Broadcast(4),
        Push(5),
        Reduce(6, "float", "min")
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

    file_tester.check("smi-host-1.h", generate_program_host(ctx.fpgas[0]))

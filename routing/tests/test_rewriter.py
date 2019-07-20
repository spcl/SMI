from ops import Push, Pop, Broadcast, Reduce, Scatter, Gather


def test_rewriter_port(rewrite_tester):
    rewrite_tester.check("port", [
        Push(0, "int"),
        Pop(3, "int")
    ])


def test_rewriter_kernel_attribute(rewrite_tester):
    rewrite_tester.check("kernel-attribute", [
        Push(0, "int"),
        Push(1, "int")
    ])


def test_rewriter_constant_variable(rewrite_tester):
    rewrite_tester.check("constant-variable", [
        Push(5, "int"),
    ])


def test_rewriter_data_type(rewrite_tester):
    rewrite_tester.check("data-type", [
        Push(0, "char"),
        Pop(1, "double"),
        Pop(3, "float"),
        Pop(4, "int")
    ])


def test_rewriter_buffer_size(rewrite_tester):
    rewrite_tester.check("buffer-size", [
        Push(0, "int", 128),
    ])


def test_rewriter_complex(rewrite_tester):
    rewrite_tester.check("complex", [
        Push(0, "char"),
        Pop(1, "double"),
        Broadcast(2, "int"),
        Reduce(3, "float", op_type="add"),
        Scatter(4, "short"),
        Gather(5, "int"),
    ])


def test_rewriter_reduce(rewrite_tester):
    rewrite_tester.check("reduce", [
        Reduce(0, op_type="add"),
        Reduce(1, op_type="min"),
        Reduce(2, op_type="max"),
    ])

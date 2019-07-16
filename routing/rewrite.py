import os
import shutil
import subprocess

from ops import Push, Pop, Broadcast, Scatter, Reduce, Gather


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
        "scatter": Scatter,
        "reduce": Reduce,
        "gather": Gather
    }
    port = int(items[1])
    return mapping[items[0]](port, *items[2:])


def rewrite(rewriter, file, include_dirs, log):
    log.write("Rewriting {}".format(file))

    args = [rewriter, file]
    for dir in include_dirs:
        args += ["-extra-arg=-I{}".format(dir)]

    process = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output = process.stdout.decode()

    log.write("STDOUT\n{}".format(output))
    log.write("STDERR\n{}".format(process.stderr.decode()))

    ops = []
    for line in output.splitlines():
        if line:
            ops.append(parse_op(line.strip()))

    return ops

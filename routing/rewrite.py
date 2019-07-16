import os
import shutil
import subprocess

from ops import Push, Pop, Broadcast, Scatter


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
        "scatter": Scatter
    }
    port = int(items[1])
    return mapping[items[0]](port, *items[2:])


def rewrite(rewriter, file, include_dirs):
    args = [rewriter, file]
    for dir in include_dirs:
        args += ["-extra-arg=-I{}".format(dir)]

    process = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output = process.stdout.decode()

    ops = []
    for line in output.splitlines():
        if line:
            ops.append(parse_op(line.strip()))

    return ops

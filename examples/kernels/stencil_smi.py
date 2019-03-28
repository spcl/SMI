import argparse
import os
import string

parser = argparse.ArgumentParser()
parser.add_argument("px", type=int)
parser.add_argument("py", type=int)
parser.add_argument("source_dir", type=str)
parser.add_argument("binary_dir", type=str)
args = parser.parse_args()

with open(os.path.join(args.source_dir, "stencil_smi.cl.in"), "r") as in_file:
    tmpl_main = string.Template(in_file.read())
with open(os.path.join(args.binary_dir, "stencil_smi", "routing.cl"),
          "r") as in_file:
    comm_code = string.Template(in_file.read())

with open(os.path.join(args.binary_dir, "stencil_smi.cl"), "w") as out_file:
    out_file.write(tmpl_main.substitute(smi_comm_code=comm_code))

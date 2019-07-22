import argparse
import os
import string

parser = argparse.ArgumentParser()
parser.add_argument("px", type=int)
parser.add_argument("py", type=int)
parser.add_argument("source_dir", type=str)
parser.add_argument("binary_dir", type=str)
args = parser.parse_args()

with open(os.path.join(args.source_dir, "stencil_onchip.cl.in"),
          "r") as in_file:
    tmpl_main = string.Template(in_file.read())
with open(os.path.join(args.source_dir, "stencil_onchip_pe.cl.in"),
          "r") as in_file:
    tmpl_pe = string.Template(in_file.read())

pe_code = []
for i_px in range(args.px):
    for i_py in range(args.py):
        pe_code.append(
            tmpl_pe.substitute(
                i_px=i_px, i_py=i_py, suffix="_{}_{}".format(i_px, i_py)))

with open(os.path.join(args.binary_dir, "stencil_onchip.cl"),
          "w") as out_file:
    out_file.write(tmpl_main.substitute(code="\n\n".join(pe_code)))

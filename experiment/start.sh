#!/usr/bin/env bash

ROUTING_DIR=routing
BINARY=../build/host_example4

python ../routing/main.py build fpga0014.txt ${ROUTING_DIR} || exit 1
mpirun --hostfile ${ROUTING_DIR}/hostfile ${BINARY} -b "/scratch/hpc-prf-mfpga/mpi-fpga/full_binaries_18.1/example4_rank<rank>_chan0_chan3.aocx" -n 64 -r ${ROUTING_DIR} || exit 1

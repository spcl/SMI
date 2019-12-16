<img align="left" width="234" height="96" src="/misc/smi.png?raw=true">

# Streaming Message Interface

**Streaming Message Interface** is a a distributed memory HLS programming model for **FPGAs** that provides
the convenience of message passing for HLS-programmed hardware devices. Instead of bulk transmission, typical of message passing model, 
with SMI messages are **streamed** across the network during computation, allowing communication to be seamlessly integrated into pipelined designs.

This repository contains an high-level synthesis implementation of SMI targeting OpenCL and Intel FPGAs, and all the 
applications used for the evaluation perfomed in the paper: *"Streaming Message Interface: High-Performance Distributed Memory
Programming on Reconfigurable Hardware"*, Tiziano De Matteis, Johannes de Fine Licht, Jakub Beránek, and Torsten Hofler. In Proceedings of the International Conference for High Performance Computing, Networking, Storage, and Analysis, 2019 (SC 2019).


Please refer to the [wiki](https://github.com/spcl/SMI/wiki) and to the paper for a reference on how to use SMI for your own distributed FPGA programs.


## Reproducing the paper experiments

All the tests and evaluations reported in the paper have been performed on a set of Bittware 520N cards (Stratix 10),
each of them equipped with 4 network connections (QSFP modules) operating at 40Gbit/s.

### Requirements

The library depends on:

* CMake for configuration
* Intel FPGA SDK for OpenCL pro, version 18.1 ([http://fpgasoftware.intel.com/opencl/](http://fpgasoftware.intel.com/opencl/)). *Experimental: support for v19+*
* GCC (version 5+)
* An MPI implementation (e.g. OpenMPI)
* Python (version 3+)
* CLang (version 8+)

### Compilation

After cloning this repository, make sure you clone the submodule dependency, by executing the following command:

```
git submodule update --init
```

The project uses CMake for configuration. To configure the project and build the bitstreams and executables:

```bash
mkdir build
cd build
cmake ..
```
The experiments shown in the paper are organized in two subdirectories of the CMake folder, `microbenchmarks` and `examples`.

For each of them the following targets are offered:

- `make <application>_emulator` builds the emulation version of the FPGA program;
- `make <application>_host` builds the host program;
- `make <application>_<program>_aoc_report` generates the report;
- `make <application>_<program>_aoc_build` builds the hardware (can take several hours).

The applications presents in the repository are the following. For the details please refer to the paper:

**Microbenchmarks**

- `bandwidth`: bandwidth microbenchmark: an MPMD application composed by two programs, namely `bandwidth_0` (sender) and `bandwidth_1` (receiver);
- `latency`: latency microbenchmark: an MPMD application composed by two programs, namely `latency_0` (source) and `latency_1` (destination).
- `injection`: injection microbenchmark: an MPMD application composed by two programs, namely `injection_0` (sender) and `injection_1` (receiver).
- `broadcast`: broadcast microbenchmark: an SPMD application (`broadcast`)
- `reduce`: reduce microbenchmark:  an SPMD application (`reduce`)
- `scatter`: scatter microbenchmark (not included in the paper): an SPMD application (`scatter`)
- `gather`: gather microbenchmark (not included in the paper):  an SPMD application (`gather`)

**Application examples**

- `stencil_smi`: stencil application, smi implementation. It is composed by a single program (`stencil_smi`);
- `stencil_onchip`: on chip version of the stencil application;
- `gesummv_smi`: gesummv, smi implementation: composed by a two programs (`gesummv_rank0` and `gesummv_rank1`);
- `gesummv_onchip`: on chip version of the gesummv application.


**Please Note**: all the host programs have been written by considering the target architecture used in the paper, which is characterized by a set of nodes each one having 2 FPGAs.
If you are using a different setup, please adjust the host programs.

### Example 

Suppose that the user wants to execute the `stencil_smi` application in emulation.
The following steps must be performed:

```bash
cd examples
# Compile the emulation version
make stencil_smi_emulator -j
# Compile the host program
make stencil_smi_host
cd stencil_smi
# Execute the program
env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 ./stencil_smi_host emulator <num-timesteps>
```

To generate the report, from the `examples` directory in the CMake folder, the user must execute:
```bash
make  stencil_smi_stencil_smi_aoc_report
```

The report will be stored under `examples/stencil_smi/stencil_smi`.



#### Stencil parameters

For the stencil application, the stencil sizes and number of ranks in either dimension are configured using CMake parameters:

```bash
cmake .. -DSMI_STENCIL_SIZE_X=8192 -DSMI_STENCIL_SIZE_Y=8192 -DSMI_STENCIL_NUM_PROCS_X=2 -DSMI_STENCIL_NUM_PROCS_Y=2
```

Other parameters include `SMI_VECTORIZATION_WIDTH`, `SMI_DATATYPE`, `SMI_FMAX`, and `SMI_ROUTING_FILE`.



<img align="left" width="312" height="128" src="/misc/smi.png?raw=true">

# Streaming Message Interface

Streaming Message Information
This README includes all the information for compiling and reproducing the results shown in the paper: 

Please refer to XXX for the descirption on how to use SMI for your own distributed FPGA program.

If you are going to use SMI for a paper or a report, please cite us:



## Reproducing  the paper experiments

### Requirements

The library depends on:

* CMake for configuration 
* Intel FPGA SDK for OpenCL pro, version 18+ ([http://fpgasoftware.intel.com/opencl/](http://fpgasoftware.intel.com/opencl/))
* GCC (version 5+)
* An MPI implementation (for the paper results we use OpenMPI 3.1.1)

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

- `make <application>_emulator` build the emulator
- `make <application>_host` build the host file
- `make <application>_<program>_aoc_emulator` build the report
- `make <application>_<program>_aoc_build` build the hardware (can take several hours)

The offered applications are:

**Microbenchmarks**

- `bandiwidth`: bandwidth microbenchmark. It is an MPMD application composed by two programs, namely `bandwidth_0` (sender) and `bandwidth_1` (receiver).
- `latency`: latency microbenchmark. It is an MPMD application composed by two programs, namely `latency_0` (source) and `latency_1` (destination).
- `injection`: injection microbenchmark.It is an MPMD application composed by two programs, namely `injection_0` (sender) and `injection_1` (receiver).
- `broadcast`: broadcast microbenchmark: it is an SPMD application (`broadcast`)
- `reduce`: reduce microbenchmark: it is an SPMD application (`reduce`)
- `scatter`: scatter microbenchmark (not included in the paper): it is an SPMD application (`scatter`)
- `gather`: gather microbenchmark (not included in the paper): it is an SPMD application (`gather`)

** Application examples**

- `stencil_smi`: stencil application, smi implementation. It is composed by a single program (`stencil_smi`);
- `stencil_onchip`: on chip version of the stencil application;
- `gesummv_smi`: gesummv, smi implementation It is composed by a two programs (`gesummv_rank0` and `gesummv_rank1`);
- `gesummv_onchip`: on chip version of the gesummv application.



**Please Note**: all the host programs have been written by considering the target architecture used in the paper, which is characterized by a set of nodes each one having 2 FPGAs.
If you are using a different setup, please adjust the host programs.

### Example 

Suppose that you want to execute the `stencil_smi` application in emulation.
The following steps must be performed: 
```bash
cd examples
make stencil_smi_emulator -j
make stencil_smi_host
cd stencil_smi
env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 ./stencil_smi_host emulator <num-timesteps
```

To generate the report, from the `examples` directory in the CMake folder, the user must execute:
```bash
make  stencil_smi_stencil_smi_aoc_report
```

The report will be stored under `examples/stencil_smi/stencil_smi`.



### Stencil parameters

The stencil sizes and number of ranks in either dimension are configured using CMake parameters:

```bash
cmake .. -DSMI_STENCIL_SIZE_X=8192 -DSMI_STENCIL_SIZE_Y=8192 -DSMI_STENCIL_NUM_PROCS_X=2 -DSMI_STENCIL_NUM_PROCS_Y=2
```

Other parameters include `SMI_VECTORIZATION_WIDTH`, `SMI_DATATYPE`, `SMI_FMAX`, and `SMI_ROUTING_FILE`.

## Using SMI

Coming soon...

# Streaming Message Interface

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

The project uses CMake for configuration. To configure the project and build the host-side executables:

```bash
mkdir build
cd build
cmake .. -DSMI_TARGET_BOARD=p520_max_sg280l"
make
```

The benchmarks shown in the paper will be built in the `examples` subdirectory of the CMake build folder. To build for the FPGA, there are three targets for each kernel:

```bash
make build_<kernel>_emulator
make build_<kernel>_report
make build_<kernel>_hardware
```

The first command generates emulation kernels used to test correctness, the second performs HLS and generates the Intel FPGA OpenCL SDK report of the synthesized design, and the third compiles for hardware (can take 12 hours). 

### Running the code

Each target has an executable capable of executing both emulation and hardware kernels, specified at runtime:

```bash
cd examples
./stencil_simple.exe emulator 32
```

For targets executing on multiple ranks, `mpirun` is used:

```bash
mpirun -np 4 ./stencil_smi_smi.exe emulator 32
```

With slurm targeting multiple FPGAs per node, in this case targeting 2 nodes with 2 FPGAs each for a total of 4 FPGAs (as reported in the paper):

```bash
srun -N 2 --tasks-per-node=2 --partition=fpga ./stencil_smi_interleaved.exe hardware 32
```

### Paper benchmarks

For the paper benchmarks, the stencil sizes are adjusted with the parameters above.
The different stencil implementations are (all with vectorization width 16, and 2x2 procs in X and Y):
- One bank: `stencil_simple.exe`
- Four banks: `stencil_spatial_tiling.exe`
- One bank, four FPGAs: `stencil_smi.exe`
- Four banks, four FPGAs: `stencil_smi_interleaved.exe`
Each kernel is built using the flow above, and the kernels are executed as specified above.

### Stencil parameters

The stencil sizes and number of ranks in either dimension are configured using CMake parameters:

```bash
cmake .. -DSMI_STENCIL_SIZE_X=8192 -DSMI_STENCIL_SIZE_Y=8192 -DSMI_STENCIL_NUM_PROCS_X=2 -DSMI_STENCIL_NUM_PROCS_Y=2
```

Other parameters include `SMI_VECTORIZATION_WIDTH`, `SMI_DATATYPE`, `SMI_FMAX`, and `SMI_ROUTING_FILE`.

## Using SMI

Coming soon...

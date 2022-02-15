This directory contains a set of unit test for the different
communication primitives. Tests are executed in emulation environment,
therefore passing the test does not ensure that the primitive will work
in hardware (but in any case it is a necessary condition)

The user can compile and runn all of them with
```
```


Tested primitives:
- p2p: point to point communications
- broadcast
- scatter
- gather
- reduce
- mixed: p2p and collective communications in the same bitstream

Each primitive is tested against different message lenght, data types and (in case of collective)
different roots.


All the tests have a timeout to ensure that a deadlock would not stall the testing procedure.
However it should be noticed that emulation can be slow, so in case try to re-execute the test
or increase the timeout.
For each test, the timeout value is defined as macro at the begining of the respective .cpp file.


To test a primitive, in the `test` folder of the Cmake folder:

1. compile the emulated bitstream

    `make test_<primitive>_emulator`

2. compile the test program

    `make test_<primitive>_host`

3. execute the test program from the respective working directory `test_<primitive>/`

    `env  CL_CONFIG_CPU_EMULATE_DEVICES=8 mpirun -np 8 ./test_<primitive>_host`

    or simply use the integration with `ctest`




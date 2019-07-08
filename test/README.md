This directory contains a set of unit test for the different
communication primitives. Tests are executed in emulation environment,
therefore passing the test does not ensure that the primitive will work
in hardware (but in any case it is a necessary condition)

The user can compile and runn all of them with
```
```


Tested primitives:
- p2p
- broadcast
- scatter
- gather
- reduce

Each primitive is tested against different message lenght, data types and (in case of collective)
different roots.


All the tests have a timeout to ensure that a deadlock would not stall the testing procedure.
However it should be noticed that emulation can be slow, so in case try to re-execute the test
or increase the timeout.
For each test, the timeout value is defined as macro at the begining of the respective .cpp file.


Otherwise she can selectively choose a primitive to test:
1. compile the emulated bitstream

    `make build_test_<primitive>_emulator`

2. compile the test program

    `make test_<primitive>.exe`

3. execute the test program

    `env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 ./test_<primitive>.exe "./<primitive>_emulator_<rank>.aocx"`

    where the last `<rank>` must be explictly written like this


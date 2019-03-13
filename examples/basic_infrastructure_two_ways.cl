/**
    In this example we have 1 sender (FPGA_0) and 2 receivers (FPGA_1)

                    -------> FPGA_1
                    |
        FPGA_0  ----
                    |
                    -------> FPGA_2

    The sender will be allocated on its own FPGA and communicates
    to the two receiver (a stream of ints)
    There will be two CK_S in the FPGA, that are connected each other.
    Beyond these connections, CK_S_0 that has two input channel from the
    sender and it is connected to the i/o channel directed to FPGA_1.
    CK_S_1 that is connected to FPGA_2
    Therefore, when CK_S_0 receives messages directed to FPGA_2, it will
    send them to CK_S_1

*/





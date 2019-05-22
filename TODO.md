#TODOs

## Implementation

- unrolling `push`/`pop` primitives: it works but the compiler inserts SERIAL EXE in some loop
	resulting in a slow execution time

- broadcast: doesnt' act as a barrier. Could it be a problem? Consider the MM case

- quitter: how to stop CK_S and CK_R. Currently they are a sort of autorun kernel (they run with a `while(1)`)
	but this can not prevent the host application to stop before all the data has been sent?
	Dunno: to ckeck. In emulation it doesn't work!!!!
	In case, one can have two additional "quitter kernels" that send the stop signal (a message) using a 
	dedicate channel to all CK_S and CK_R and then they stop. This are launched at the end of the computation?
	But, even in that case it is a problem...
	Or can we rely on MPI_FINALIZE?


- code generation MPMD: if the ranks are asymettric (e.g. one send and the other receive) we need to differentiate between "sending" and "receiving" tags

- API: uniform everything with channel_id. Problem with collectives: not able to solve the ch_id as a constant. We
	can move this to the support kernel, but remains the problem of the data type and the unrolling

- CK_S/CK_R: keep reading from the same interface

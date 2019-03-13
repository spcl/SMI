#TODOs

## Implementation

- unrolling in `push` primitives: into the code there is an unrolled loop to memcpy the datum.
	The number of loop iterations depends on the data size, which is stored into the 
	channel descriptor. Apparetenly, v 18.1.1 of the compiler is not able to unroll this.
	If this is confirmed, one can explictly have multiple `if` with the unrolling


- quitter: how to stop CK_S and CK_R. Currently they are a sort of autorun kernel (they run with a `while(1)`)
	but this can not prevent the host application to stop before all the data has been sent?
	Dunno: to ckeck. In emulation it doesn't work!!!!
	In case, one can have two additional "quitter kernels" that send the stop signal (a message) using a 
	dedicate channel to all CK_S and CK_R and then they stop. This are launched at the end of the computation?
	But, even in that case it is a problem...
	Or can we rely on MPI_FINALIZE?


#TODOs

## Implementation

- unrolling in `push` primitives: into the code there is an unrolled loop to memcpy the datum.
	The number of loop iterations depends on the data size, which is stored into the 
	channel descriptor. Apparetenly, v 18.1.1 of the compiler is not able to unroll this.
	If this is confirmed, one can explictly have multiple `if` with the unrolling



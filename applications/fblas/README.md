#README

With this application use case we want to evaluate two possible implementation of GESUMMV, which computes
	
	y = alpha*A*x+beta*B*y

1. in the first version we evaluate a streamed on-chip implementation, in which the two GEMV
	are composed together. The output of the computation of beta*B*y is streamed
	towards the second GEMV, since it is an admissible composition.
	Each of the two GEMVs will read its own matrix from a different memory module

2. distributed implementation:
	- on rank 0 we perform alpha*A*x: the result is streamed to rank 1
	- on rank 1 we perform beta*B*y: the result is combined with the result
		of rank 0 to produce the final y
	
    For fairness, each of the two GEMVs will use two modules/.
    The result is streamed using SMI


## Technical notes
Since the compiler do not automatically interleave buffer across multiple memory banks
we manually interleave it.


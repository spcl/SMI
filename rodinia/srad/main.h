#ifdef FP_DOUBLE
#define fp double
#else
#define fp float
#endif

#ifdef RD_WG_SIZE_0_0
        #define NUMBER_THREADS RD_WG_SIZE_0_0
#elif defined(RD_WG_SIZE_0)
        #define NUMBER_THREADS RD_WG_SIZE_0
#elif defined(RD_WG_SIZE)
        #define NUMBER_THREADS RD_WG_SIZE
#else
        #define NUMBER_THREADS 256
#endif

// for all versions
#ifndef RED_UNROLL
	#define RED_UNROLL 8
#endif

// for v5
#define HALO 			2 // this kernel has two stencils; hence, halo size is 2 instead of 1

#ifndef BSIZE
  #define BSIZE 4096
#endif

#ifndef SSIZE
  #define SSIZE 16
#endif

#define AMB_TEMP (80.0f)

#define WG_SIZE_X (64)
#define WG_SIZE_Y (4)

// Block size
#ifndef BSIZE
  // From Zohouri's PhD thesis, page 41
  #define BSIZE 512
#endif

#define BLOCK_X BSIZE
#define BLOCK_Y BSIZE

// Vector size
#ifndef SSIZE
  // From Zohouri's PhD thesis, page 41
  #define SSIZE 16
#endif

// Radius of stencil, e.g 5-point stencil => 1
#ifndef RAD
  #define RAD  1
#endif

// Number of parallel time steps
// (from Zohouri's FPGA'18 paper, not used in v5)
#ifndef TIME
  #define TIME 21
#endif

// Padding to fix alignment for time steps that are not a multiple of 8
#ifndef PAD
  #define PAD TIME % 16
#endif

#define HALO_SIZE		TIME * RAD			// halo size
#define BACK_OFF		2 * HALO_SIZE			// back off for going to next block

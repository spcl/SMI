#define AMB_TEMP (80.0f)

// Block size
#ifndef BSIZE
#define BSIZE 512
#endif

#ifndef BLOCK_X
#define BLOCK_X 16
#endif

#ifndef BLOCK_Y
#define BLOCK_Y 16
#endif

// Vector size
#ifndef SSIZE
#define SSIZE 8
#endif

// Radius of stencil, e.g 5-point stencil => 1
#ifndef RAD
#define RAD 1
#endif

// Number of parallel time steps
#ifndef TIME
#define TIME 21
#endif

// Padding to fix alignment for time steps that are not a multiple of 8
#ifndef PAD
#define PAD TIME % 16
#endif

#define HALO_SIZE TIME* RAD     // halo size
#define BACK_OFF 2 * HALO_SIZE  // back off for going to next block

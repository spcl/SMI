#include "stencil.h"

channel VTYPE read_stream __attribute__((depth(Y/W)));
channel VTYPE write_stream __attribute__((depth(Y/W)));

kernel void Read(global volatile const VTYPE memory[], const int timesteps) {
  #pragma loop_coalesce
  for (int t = 0; t < timesteps; ++t) {
    int offset = (t % 2 == 0) ? 0 : X * (Y / W);
    for (int i = 0; i < X; ++i) {
      for (int j = 0; j < Y / W; ++j) {
        VTYPE read = memory[offset + i * (Y / W) + j];
        write_channel_intel(read_stream, read);
      }
    }
  }
}

kernel void Write(global volatile VTYPE memory[], const int timesteps) {
  #pragma loop_coalesce
  for (int t = 0; t < timesteps; ++t) {
    int offset = (t % 2 == 0) ? X * (Y / W) : 0;
    for (int i = 1; i < X - 1; ++i) {
      for (int j = 0; j < (Y / W); ++j) {
        VTYPE read = read_channel_intel(write_stream);
        memory[offset + i * (Y / W) + j] = read;
      }
    }
  }
}

kernel void Stencil(const int timesteps) {

  DTYPE buffer[2 * HALO_X * Y + W];

  #pragma loop_coalesce
  for (int t = 0; t < timesteps; ++t) {
    for (int i = 0; i < X; ++i) {
      for (int j = 0; j < (Y / W); ++j) {

        #pragma unroll
        for (int i = 0; i < 2 * HALO_X * Y; i++) {
          buffer[i] = buffer[i + W];
        }

        VTYPE read = read_channel_intel(read_stream);

#if W > 1
        #pragma unroll
        for (int w = 0; w < W; w++) {
          buffer[2 * HALO_X * Y + w] = read[w];
        }
#else
        buffer[2 * HALO_X * Y] = read;
#endif

        if (i >= 2) {

          VTYPE res;
          #pragma unroll
          for (int w = 0; w < W; w++) {
            DTYPE value;
            // If out of bounds, just put whatever value was here before
            if (j * W + w < HALO_Y || j * W + w >= Y - HALO_Y) {
              value = buffer[HALO_X * Y + w];
            } else {
              value = 0.25f *
                      (buffer[HALO_X * Y + w - 1] + buffer[HALO_X * Y + w + 1] +
                       buffer[2 * HALO_X * Y + w] + buffer[w]);
            }
#if W > 1
            res[w] = value;
#else
            res = value;
#endif
          }

          write_channel_intel(write_stream, res);
        }

      }
    }
  }
}

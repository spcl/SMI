#include "stencil.h"

channel float read_stream __attribute__((depth(Y)));
channel float write_stream __attribute__((depth(Y)));

__kernel void Read(__global volatile const float memory[],
                   const int timesteps) {
  for (int t = 0; t < timesteps; ++t) {
    int offset = (t % 2 == 0) ? 0 : X * Y;
    for (int i = 0; i < X; ++i) {
      for (int j = 0; j < Y; ++j) {
        DTYPE read = memory[offset + i * Y + j];
        write_channel_intel(read_stream, read);
      }
    }
  }
}

__kernel void Stencil(const int timesteps) {
  for (int t = 0; t < timesteps; ++t) {
    DTYPE buffer[2 * Y + 1];
    for (int i = 0; i < X; ++i) {
      for (int j = 0; j < Y; ++j) {
        // Shift buffer
        #pragma unroll
        for (int b = 0; b < 2 * Y; ++b) {
          buffer[b] = buffer[b + 1];
        }
        // Read into front
        buffer[2 * Y] = read_channel_intel(read_stream);
        if (i >= 2 && j >= 1 && j < Y - 1) {
          DTYPE res = 0.25 * (buffer[2 * Y] + buffer[Y - 1] + buffer[Y + 1] +
                              buffer[0]);
          write_channel_intel(write_stream, res);
        }
      }
    }
  }
}

__kernel void Write(__global volatile float memory[], const int timesteps) {
  for (int t = 0; t < timesteps; ++t) {
    int offset = (t % 2 == 0) ? X * Y : 0;
    for (int i = 1; i < X - 1; ++i) {
      for (int j = 1; j < Y - 1; ++j) {
        DTYPE read = read_channel_intel(write_stream);
        memory[offset + i * Y + j] = read;
      }
    }
  }
}

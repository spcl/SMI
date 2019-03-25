#include "stencil.h"

// channel VTYPE read_stream __attribute__((depth(Y)));
// channel VTYPE write_stream __attribute__((depth(Y)));
//
// __kernel void Read(__global volatile const VTYPE memory[],
//                    const int timesteps) {
//   #pragma loop_coalesce
//   for (int t = 0; t < timesteps; ++t) {
//     int offset = (t % 2 == 0) ? 0 : X * (Y / W);
//     for (int i = 0; i < X; ++i) {
//       for (int j = 0; j < Y / W; ++j) {
//         VTYPE read = memory[offset + i * (Y / W) + j];
//         write_channel_intel(read_stream, read);
//       }
//     }
//   }
// }
//
// __kernel void Stencil(const int timesteps) {
//   #pragma loop_coalesce
//   for (int t = 0; t < timesteps; ++t) {
//     DTYPE buffer[2 * Y + W];
//     for (int i = 0; i < X; ++i) {
//       for (int j = 0; j < Y / W; ++j) {
//         // Shift buffer
//         #pragma unroll
//         for (int b = 0; b < 2 * Y; ++b) {
//           buffer[b] = buffer[b + W];
//         }
//         // Read into front
//         VTYPE read = read_channel_intel(read_stream);
//         #pragma unroll
//         for (int w = 0; w < W; ++w) {
//           buffer[2 * Y + w] = read[w];
//         }
//         if (i >= 2 && j >= 1 && j < Y / W - 1) {
//           VTYPE res;
//           #pragma unroll
//           for (int w = 0; w < W; ++w) {
//             res[w] = 0.25 * (buffer[2 * Y + w] + buffer[Y - 1 + w] +
//                              buffer[Y + 1 + w] + buffer[w]);
//           }
//           write_channel_intel(write_stream, res);
//         }
//       }
//     }
//   }
// }
//
// __kernel void Write(__global volatile VTYPE memory[], const int timesteps) {
//   #pragma loop_coalesce
//   for (int t = 0; t < timesteps; ++t) {
//     int offset = (t % 2 == 0) ? X * (Y / W) : 0;
//     for (int i = 1; i < X - 1; ++i) {
//       for (int j = 1; j < (Y / W) - 1; ++j) {
//         VTYPE read = read_channel_intel(write_stream);
//         memory[offset + i * (Y / W) + j] = read;
//       }
//     }
//   }
// }

__kernel void Stencil(__global volatile VTYPE const* restrict input,
                      __global volatile VTYPE* restrict output,
                      const int timesteps) {
  #pragma loop_coalesce
  for (int t = 0; t < timesteps; ++t) {
    DTYPE buffer[2 * Y + W];
    for (int i = 0; i < X; ++i) {
      for (int j = 0; j < Y / W; ++j) {
        // Shift buffer
        #pragma unroll
        for (int b = 0; b < 2 * Y; ++b) {
          buffer[b] = buffer[b + W];
        }
        // Read into front
        VTYPE read = input[i * (Y / W) + j];
        #pragma unroll
        for (int w = 0; w < W; ++w) {
          buffer[2 * Y + w] = read[w];
        }
        if (i >= 2 && j >= 1 && j < Y / W - 1) {
          VTYPE res;
          #pragma unroll
          for (int w = 0; w < W; ++w) {
            res[w] = 0.25 * (buffer[2 * Y + w] + buffer[Y - 1 + w] +
                             buffer[Y + 1 + w] + buffer[w]);
          }
          output[(i - 1) * (Y / W) + j] = res;
        }
      }
    }
  }
}

#include "hotspot.h"
#include "smi.h"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

//#define DEBUG
#ifdef DEBUG
#  define D(x) x
#else
#  define D(x)
#endif

#if PX * PY != RANK_COUNT
#error "Incompatible number of stencil processes and number of communication ranks."
#endif

//channel VTYPE read_stream[B] __attribute__((depth((Y/(W*B))/PY)));
//channel VTYPE power_stream[B] __attribute__((depth((Y/(W*B))/PY)));
//channel VTYPE write_stream[B] __attribute__((depth((Y/(W*B))/PY)));
//channel VTYPE receive_top[B] __attribute__((depth((Y/(W*B))/PY)));
//channel VTYPE receive_bottom[B] __attribute__((depth((Y/(W*B))/PY)));
//channel float receive_left __attribute__((depth(X/PX)));
//channel float receive_right __attribute__((depth(X/PX)));
//channel VTYPE send_top[B] __attribute__((depth((Y/(W*B))/PY)));
//channel VTYPE send_bottom[B] __attribute__((depth((Y/(W*B))/PY)));
//channel float send_left __attribute__((depth(X/PX)));
//channel float send_right __attribute__((depth(X/PX)));

channel VTYPE read_stream[B] __attribute__((depth(2)));
channel VTYPE power_stream[B] __attribute__((depth(2)));
channel VTYPE write_stream[B] __attribute__((depth(2)));
channel VTYPE receive_top[B] __attribute__((depth(2)));
channel VTYPE receive_bottom[B] __attribute__((depth(2)));
channel float receive_left __attribute__((depth(32)));
channel float receive_right __attribute__((depth(32)));
channel VTYPE send_top[B] __attribute__((depth(2)));
channel VTYPE send_bottom[B] __attribute__((depth(2)));
channel float send_left __attribute__((depth(32)));
channel float send_right __attribute__((depth(32)));

kernel void Read(__global volatile const VTYPE *restrict bank0,
                 __global volatile const VTYPE *restrict bank1,
                 __global volatile const VTYPE *restrict bank2,
                 __global volatile const VTYPE *restrict bank3, const int i_px,
                 const int i_py, const int timesteps, char my_rank) {
  // Extra artificial timestep to send first boundaries
  for (int t = 0; t < timesteps + 1; ++t) {
    D(printf(ANSI_COLOR_GREEN "[READ-%d] timestemp: %d\n" ANSI_COLOR_RESET,my_rank,t);)
    // Swap the timestep modulo to accommodate first artificial timestep
    const int offset =
        (t == 0 || t % 2 == 1) ? 0 : X_LOCAL * (Y_LOCAL / (B * W));
    // +1 for each boundary
    for (int i = 0; i < X_LOCAL + 2; ++i) {
      // +1 for each boundary
      for (int j = 0; j < (Y_LOCAL / (B * W)) + 2; ++j) {
        VTYPE res[B];
        #pragma unroll
        for (int b = 0; b < B; ++b) {
          res[b] = -100;
        }
        // oob: out of bounds with respect to the local domain, i.e., not
        // necessarily the global domain.
        const bool oob_top = i < 1;
        const bool oob_bottom = i >= X_LOCAL + 1;
        // It's assumed that the horizontal halo fits in one vector, such that
        // HALO_Y <= W (hence, we use 1 here).
        const bool oob_left = j < 1;
        const bool oob_right = j >= (Y_LOCAL / (B * W)) + 1;
        const bool valid_read =
            !oob_top && !oob_bottom && !oob_left && !oob_right;
        if (valid_read) {
          res[0] =
              bank0[offset + (i - 1) * (Y_LOCAL / (W * B)) + (j - 1)];
          res[1] =
              bank1[offset + (i - 1) * (Y_LOCAL / (W * B)) + (j - 1)];
          res[2] =
              bank2[offset + (i - 1) * (Y_LOCAL / (W * B)) + (j - 1)];
          res[3] =
              bank3[offset + (i - 1) * (Y_LOCAL / (W * B)) + (j - 1)];
        } else {
          // We don't need to communicate on the corner, as this is not used by
          // the stencil.
          const bool on_corner =
              (oob_top && oob_left) || (oob_top && oob_right) ||
              (oob_bottom && oob_left) || (oob_bottom && oob_right);
          if (oob_top) {
            if (i_px > 0 && t > 0 && !on_corner) {
              // Read from channel above
              D(printf(ANSI_COLOR_GREEN "[READ-%d] I need to read from above\n" ANSI_COLOR_RESET,my_rank);)
              #pragma unroll
              for (int b = 0; b < B; ++b) {
                res[b] = read_channel_intel(receive_top[b]);
              }
              D(printf(ANSI_COLOR_GREEN"[READ-%d] Read from above\n"ANSI_COLOR_RESET,my_rank);)
            }
          } else if (oob_bottom) {
            if (i_px < PX - 1 && t > 0 && !on_corner) {
              // Read from channel below
              D(printf(ANSI_COLOR_GREEN "[READ-%d] I need to read from below\n" ANSI_COLOR_RESET,my_rank);)
              #pragma unroll
              for (int b = 0; b < B; ++b) {
                res[b] = read_channel_intel(receive_bottom[b]);
              }
              D(printf(ANSI_COLOR_GREEN "[READ-%d] Read from below\n" ANSI_COLOR_RESET,my_rank);)
            }
          } else if (oob_left) {
            if (i_py > 0 && t > 0 && !on_corner) {
              // Read from left channel
              D(printf(ANSI_COLOR_GREEN "[READ-%d] I need to read from left\n" ANSI_COLOR_RESET,my_rank);)
              float read_horizontal = read_channel_intel(receive_left);
               D(printf(ANSI_COLOR_GREEN "[READ-%d] read from left\n" ANSI_COLOR_RESET,my_rank);)
              // Populate elements within boundary, leaving the rest
              // uninitialized/dummy values
              res[B - 1][W - 1] = read_horizontal;
            }
          } else if (oob_right) {
            if (i_py < PY - 1 && t > 0 && !on_corner) {
              // Read from right channel
               D(printf(ANSI_COLOR_GREEN "[READ-%d] I need to read from right\n" ANSI_COLOR_RESET,my_rank);)
              float read_horizontal = read_channel_intel(receive_right);
               D(printf(ANSI_COLOR_GREEN "[READ-%d] Read from right\n" ANSI_COLOR_RESET,my_rank);)
              // Populate elements within boundary, leaving the rest
              // uninitialized/dummy values
              res[0][0] = read_horizontal;
            }
          }
        } // !valid_read
        #pragma unroll
        for (int b = 0; b < B; ++b) {
          write_channel_intel(read_stream[b], res[b]);
        }
        D(printf(ANSI_COLOR_GREEN "[READ-%d] sent data (%d,%d) to stencil kernel \n" ANSI_COLOR_RESET,my_rank,i,j);)
      }
    }
  }
}

kernel void ReadPower(__global volatile const VTYPE *restrict bank0,
                      __global volatile const VTYPE *restrict bank1,
                      __global volatile const VTYPE *restrict bank2,
                      __global volatile const VTYPE *restrict bank3,
                      const int timesteps, char my_rank) {
  // Extra artificial timestep to send first boundaries
  for (int t = 0; t < timesteps + 1; ++t) {
     D(printf(ANSI_COLOR_BLUE "[POWER-%d] timestep %d\n" ANSI_COLOR_RESET,my_rank,t);)

    // +1 for each boundary
    for (int i = 0; i < X_LOCAL + 2; ++i) {
      // +1 for each boundary
      for (int j = 0; j < (Y_LOCAL / (B * W)) + 2; ++j) {
        VTYPE res[B];
        #pragma unroll
        for (int b = 0; b < B; ++b) {
          res[b] = -100;
        }
        // oob: out of bounds with respect to the local domain, i.e., not
        // necessarily the global domain.
        const bool oob_top = i < 1;
        const bool oob_bottom = i >= X_LOCAL + 1;
        // It's assumed that the horizontal halo fits in one vector, such that
        // HALO_Y <= W (hence, we use 1 here).
        const bool oob_left = j < 1;
        const bool oob_right = j >= (Y_LOCAL / (B * W)) + 1;
        const bool valid_read =
            !oob_top && !oob_bottom && !oob_left && !oob_right;
        if (valid_read) {
          res[0] = bank0[(i - 1) * (Y_LOCAL / (W * B)) + (j - 1)];
          res[1] = bank1[(i - 1) * (Y_LOCAL / (W * B)) + (j - 1)];
          res[2] = bank2[(i - 1) * (Y_LOCAL / (W * B)) + (j - 1)];
          res[3] = bank3[(i - 1) * (Y_LOCAL / (W * B)) + (j - 1)];
        }
        #pragma unroll
        for (int b = 0; b < B; ++b) {
          write_channel_intel(power_stream[b], res[b]);
        }
        D(printf(ANSI_COLOR_BLUE "[POWER-%d] sent data (%d,%d) to stencil kernel \n" ANSI_COLOR_RESET,my_rank,i,j);)

      }
    }
  }
  D(printf(ANSI_COLOR_BLUE "***********[POWER-%d] finished************ \n" ANSI_COLOR_RESET,my_rank);)

}

kernel void Stencil(const float rx1, const float ry1, const float rz1,
                    const float sdc, const int i_px, const int i_py,
                    const int timesteps,char my_rank) {
  D(printf("[STENCIL-%d] Y_LOCAL: %d, B: %d, W: %d\n",my_rank,Y_LOCAL,B,W);)
  for (int t = 0; t < timesteps + 1; ++t) {
      D(printf("[STENCIL-%d] Executing iteration %d\n",my_rank,t);)
    float buffer[2 * (Y_LOCAL + 2 * B * W) + B * W];
    for (int i = 0; i < X_LOCAL + 2; ++i) {
      for (int j = 0; j < (Y_LOCAL / (B * W)) + 2; ++j) {
        // Shift buffer
        #pragma unroll
        for (int b = 0; b < 2 * (Y_LOCAL + 2 * B * W); ++b) {
          buffer[b] = buffer[b + B * W];
        }
        // Read into front
        #pragma unroll
        for (int b = 0; b < B; ++b) {
          VTYPE read = read_channel_intel(read_stream[b]);
          #pragma unroll
          for (int w = 0; w < W; ++w) {
            buffer[2 * (Y_LOCAL + 2 * B * W) + b * W + w] = read[w];
          }
        }
        // Read power 
        VTYPE power[B];
        #pragma unroll
        for (int b = 0; b < B; ++b) {
          power[b] = read_channel_intel(power_stream[b]);
        }
        D(printf("[STENCIL-%d] read data (%d,%d)\n",my_rank,i,j);)
        // If in bounds, compute and output
        if (i >= 2 && j >= 1 && j < (Y_LOCAL / (B * W)) + 1) {
          #pragma unroll
          for (int b = 0; b < B; ++b) {
            VTYPE res;
            #pragma unroll
            for (int w = 0; w < W; ++w) {
              if (t == 0) {
                res[w] = buffer[Y_LOCAL + 2 * B * W + b * W + w];
              } else {
                float north, west, east, south;
                float center = buffer[(Y_LOCAL + 2 * B * W) + b * W + w];
                if (i_px == 0 && i < 3) {
                  north = center;
                } else {
                  north = buffer[b * W + w];
                }
                if (i_px == PX - 1 && i >= X_LOCAL + 1) {
                  south = center;
                } else {
                  south = buffer[2 * (Y_LOCAL + 2 * B * W) + b * W + w];
                }
                if (i_py == 0 && j * B * W + b * W + w < B * W + 1) {
                  west = center;
                } else {
                  west = buffer[(Y_LOCAL + 2 * B * W) + b * W + w - 1];
                }
                if (i_py == PY - 1 &&
                    j * B * W + b * W + w >= B * W + Y_LOCAL - 1) {
                  east = center;
                } else {
                  east = buffer[(Y_LOCAL + 2 * B * W) + b * W + w + 1];
                }
                const float v = power[b][w] +
                                (north + south - 2.0f * center) * ry1 +
                                (east + west - 2.0f * center) * rx1 +
                                (AMB_TEMP - center) * rz1;
                const float delta = sdc * v;
                res[w] = center + delta; 
                // if (i_px == 0 && i_py == 0) {
                //   printf("(%i, %i, %i): %f, %f, %f, %f, %f = %f\n", t - 1, i - 2,
                //          j - 1, north, west, center, east, south, res[w]);
                // }
              }
            }
            write_channel_intel(write_stream[b], res);
            D(printf("[STENCIL-%d] sent data (%d,%d)\n",my_rank,i,j);)
          }
         // printf("[STENCIL-%d] sent data for timestamp: %d j:%d\n",my_rank,t,j);
        }
      }
    }
    //mem_fence(CLK_GLOBAL_MEM_FENCE | CLK_CHANNEL_MEM_FENCE);
  }
  D(printf("**************** [Stencil-%d] finished ********** \n",my_rank);)

}

kernel void Write(__global volatile VTYPE *restrict bank0,
                  __global volatile VTYPE *restrict bank1,
                  __global volatile VTYPE *restrict bank2,
                  __global volatile VTYPE *restrict bank3, const int i_px,
                  const int i_py, const int timesteps, char my_rank) {
  // Extra timestep to write first halos before starting computation
  D(printf(ANSI_COLOR_YELLOW "[WRITE-%d] i_px: %d, i_py: %d\n" ANSI_COLOR_RESET,my_rank,i_px,i_py);)
  D(printf(ANSI_COLOR_YELLOW "[WRITE-%d] i loops runs for %d iteration, j-loop runs for %d iterations\n" ANSI_COLOR_RESET,my_rank,X_LOCAL,Y_LOCAL / (B * W));)
  for (int t = 0; t < timesteps + 1; ++t) {
    D(printf(ANSI_COLOR_YELLOW "[WRITE-%d] timestamp %d\n" ANSI_COLOR_RESET,my_rank,t);)
    // Extra artifical timestep shifts the offset
    int offset = (t % 2 == 0) ? 0 : X_LOCAL * (Y_LOCAL / (B * W));
    for (int i = 0; i < X_LOCAL; ++i) {
      D(printf(ANSI_COLOR_YELLOW "[WRITE-%d] timestamp %d, working on i: %d\n" ANSI_COLOR_RESET,my_rank,t,i);)
      for (int j = 0; j < Y_LOCAL / (B * W); ++j) {
        VTYPE read[B];
        #pragma unroll
        for (int b = 0; b < B; ++b) {
          read[b] = read_channel_intel(write_stream[b]);
        }
        if (i_px > 0 && i < 1) {
          if (t < timesteps) { // Don't communicate on last timestep
            // Write to channel above
//            printf(ANSI_COLOR_YELLOW "[WRITE-%d] I need to write on the top.......\n" ANSI_COLOR_RESET,my_rank);
            #pragma unroll
            for (int b = 0; b < B; ++b) {
              write_channel_intel(send_top[b], read[b]);
            }
//            printf(ANSI_COLOR_YELLOW ".......[WRITE-%d] I wrote the top\n" ANSI_COLOR_RESET,my_rank);
          }
        }
        if (i_px < PX - 1 && i >= X_LOCAL - 1) {
          if (t < timesteps) { // Don't communicate on last timestep
            // Write channel below
//              printf(ANSI_COLOR_YELLOW "[WRITE-%d] I need to write on the bottom.......\n" ANSI_COLOR_RESET,my_rank);
            #pragma unroll
            for (int b = 0; b < B; ++b) {
              write_channel_intel(send_bottom[b], read[b]);

            }
//            printf(ANSI_COLOR_YELLOW ".......[WRITE-%d] I wrote the bottom\n" ANSI_COLOR_RESET,my_rank);
          }
        }
        if (i_py > 0 && j < 1) {
          if (t < timesteps) { // Don't communicate on last timestep
            // Extract relevant values
            float write_horizontal = read[0][0];
            // Write to left channel
//            printf(ANSI_COLOR_YELLOW "[WRITE-%d] I need to write on the left........\n" ANSI_COLOR_RESET,my_rank);
            write_channel_intel(send_left, write_horizontal);
//            printf(ANSI_COLOR_YELLOW ".......[WRITE-%d] I wrote on the left\n" ANSI_COLOR_RESET,my_rank);
          }
        }
        if (i_py < PY - 1 && j >= (Y_LOCAL / (B * W)) - 1) {
          if (t < timesteps) {
            // Extract relevant values
              float write_horizontal = read[B - 1][W - 1];
            // Write to right channel
            //printf(ANSI_COLOR_YELLOW "[WRITE-%d] I need to write on the right.......\n" ANSI_COLOR_RESET,my_rank);
            write_channel_intel(send_right, write_horizontal);
            //printf(ANSI_COLOR_YELLOW "......[WRITE-%d] I wrote the right\n" ANSI_COLOR_RESET,my_rank);
          }
        }
       // printf(ANSI_COLOR_YELLOW "[WRITE-%d] timestamp %d, written data: %d\n" ANSI_COLOR_RESET,my_rank,t,j);

        if (t > 0) {
          bank0[offset + i * (Y_LOCAL / (B * W)) + j] = read[0];
          bank1[offset + i * (Y_LOCAL / (B * W)) + j] = read[1];
          bank2[offset + i * (Y_LOCAL / (B * W)) + j] = read[2];
          bank3[offset + i * (Y_LOCAL / (B * W)) + j] = read[3];
        }
        //mem_fence(CLK_GLOBAL_MEM_FENCE | CLK_CHANNEL_MEM_FENCE);

      }
    }
  }
  D(printf(ANSI_COLOR_YELLOW "*******************[WRITE-%d] finished*********************\n" ANSI_COLOR_RESET,my_rank);)

}

kernel void ConvertReceiveLeft(const int i_px, const int i_py, char my_rank) {
    int iteration=0;
  while (1) {

      D(printf(ANSI_COLOR_RED"[CRL - %d] starting iteration %d\n" ANSI_COLOR_RESET,my_rank,iteration);)

    SMI_Channel from_network =
        SMI_Open_receive_channel(X_LOCAL, SMI_FLOAT, i_px * PY + (i_py - 1), 1);
    for (int i = 0; i < X_LOCAL; ++i) {
      float val; 
      SMI_Pop(&from_network, &val);
      //printf("[CRL - %d] received the %d-th value, sending it\n",my_rank,i);
      mem_fence( CLK_CHANNEL_MEM_FENCE);
      write_channel_intel(receive_left, val);

    }
    D(printf(ANSI_COLOR_RED "[CRL - %d] completed iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration++);)

  }
}

kernel void ConvertReceiveRight(const int i_px, const int i_py, char my_rank) {
    int iteration=0;
  while (1) {
      D(printf(ANSI_COLOR_RED "[CRR - %d] starting iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration);)

    SMI_Channel from_network =
        SMI_Open_receive_channel(X_LOCAL, SMI_FLOAT, i_px * PY + (i_py + 1), 3);
    for (int i = 0; i < X_LOCAL; ++i) {
      float val; 
      SMI_Pop(&from_network, &val);
      //printf("[CRR - %d] received the %d-th value, sending it\n",my_rank,i);
      //mem_fence(CLK_CHANNEL_MEM_FENCE);
      write_channel_intel(receive_right, val);

    }
    D(printf(ANSI_COLOR_RED "[CRR - %d] completed iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration++);)

  }
}

kernel void ConvertReceiveTop(const int i_px, const int i_py,char my_rank) {
    int iteration=0;
  while (1) {
    D(printf(ANSI_COLOR_RED "[CRT - %d] starting iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration);)

    SMI_Channel from_network =
        SMI_Open_receive_channel(Y_LOCAL, SMI_FLOAT, (i_px - 1) * PY + i_py, 2);
    VTYPE vec[B];
    #pragma loop_coalesce
    for (int i = 0; i < Y_LOCAL / (B * W); ++i) {
      for (int b0 = 0; b0 < B; ++b0) {
        for (int w = 0; w < W; ++w) {
          float val;
          SMI_Pop(&from_network, &val);
          //printf("[CRT - %d] received the %d-th value, sending it\n",my_rank,i);
          //mem_fence(CLK_CHANNEL_MEM_FENCE);
          vec[b0][w] = val;
          if (b0 == B - 1 && w == W - 1) {
            #pragma unroll
            for (int b1 = 0; b1 < B; ++b1) {
              write_channel_intel(receive_top[b1], vec[b1]);
            }

          }
        }
      }
    }
    D(printf(ANSI_COLOR_RED"[CRT - %d] completed iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration++);)

  }
}

kernel void ConvertReceiveBottom(const int i_px, const int i_py,char my_rank) {
    int iteration=0;
  while (1) {
    D(printf(ANSI_COLOR_RED"[CRB - %d] starting iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration);)

    SMI_Channel from_network =
        SMI_Open_receive_channel(Y_LOCAL, SMI_FLOAT, (i_px + 1) * PY + i_py, 0);
    VTYPE vec[B];
    #pragma loop_coalesce
    for (int i = 0; i < Y_LOCAL / (B * W); ++i) {
      for (int b0 = 0; b0 < B; ++b0) {
        for (int w = 0; w < W; ++w) {
          float val;
          SMI_Pop(&from_network, &val);
          //printf("[CRB - %d] received the %d-th value, sending it\n",my_rank,i);
          //mem_fence(CLK_CHANNEL_MEM_FENCE);

          vec[b0][w] = val;
          if (b0 == B - 1 && w == W - 1) {
            #pragma unroll
            for (int b1 = 0; b1 < B; ++b1) {
              write_channel_intel(receive_bottom[b1], vec[b1]);
            }
            //mem_fence(CLK_GLOBAL_MEM_FENCE | CLK_CHANNEL_MEM_FENCE);

          }
        }
      }
    }
    D(printf(ANSI_COLOR_RED "[CRB - %d] completed iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration++);)

  }
}

kernel void ConvertSendLeft(const int i_px, const int i_py, char my_rank) {
    int iteration=0;
  while (1) {
    SMI_Channel to_network =
        SMI_Open_send_channel(X_LOCAL, SMI_FLOAT, i_px * PY + i_py - 1, 3);
    D(printf(ANSI_COLOR_RED"[CSL - %d] starting iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration);)
    for (int i = 0; i < X_LOCAL; ++i) {
      //printf("[CSL - %d] Waiting for a value...\n",my_rank);
      float val = read_channel_intel(send_left);
      //printf("[CSL - %d] send the %d-th value, sending it\n",my_rank,i);
      //mem_fence(CLK_GLOBAL_MEM_FENCE | CLK_CHANNEL_MEM_FENCE);
     // mem_fence(CLK_CHANNEL_MEM_FENCE);

      SMI_Push(&to_network, &val);

    }
    D(printf(ANSI_COLOR_RED "[CSL - %d] completed iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration);)
    iteration++;

  }
}

kernel void ConvertSendRight(const int i_px, const int i_py,char my_rank) {
    int iteration=0;
  while (1) {
    D(printf(ANSI_COLOR_RED "[CSR - %d] starting iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration);)

    SMI_Channel to_network =
        SMI_Open_send_channel(X_LOCAL, SMI_FLOAT, i_px * PY + i_py + 1, 1);
    for (int i = 0; i < X_LOCAL; ++i) {
      // printf("[CSR - %d] Waiting for a value...\n",my_rank);
      float val = read_channel_intel(send_right);
      //printf("[CSR - %d] send the %d-th value, sending it\n",my_rank,i);
      //mem_fence(CLK_GLOBAL_MEM_FENCE | CLK_CHANNEL_MEM_FENCE);
     // mem_fence(CLK_CHANNEL_MEM_FENCE);

      SMI_Push(&to_network, &val);

    }
    D(printf(ANSI_COLOR_RED "[CSR - %d] completed iteration %d\n" ANSI_COLOR_RESET,my_rank,iteration++);)

  }
}

kernel void ConvertSendTop(const int i_px, const int i_py,char my_rank) {
    int iteration=0;
  while (1) {
      D(printf(ANSI_COLOR_RED"[CST - %d] starting iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration);)
    SMI_Channel to_network =
        SMI_Open_send_channel(Y_LOCAL, SMI_FLOAT, (i_px - 1) * PY + i_py, 0);
    VTYPE vec[B];
    #pragma loop_coalesce
    for (int i = 0; i < Y_LOCAL / (B * W); ++i) {
      for (int b0 = 0; b0 < B; ++b0) {
        for (int w = 0; w < W; ++w) {
          if (b0 == 0 && w == 0) {
            #pragma unroll
            for (int b1 = 0; b1 < B; ++b1) {
              vec[b1] = read_channel_intel(send_top[b1]);
            }
          }
          //mem_fence(CLK_GLOBAL_MEM_FENCE | CLK_CHANNEL_MEM_FENCE);

          float val = vec[b0][w];
        //  mem_fence(CLK_CHANNEL_MEM_FENCE);

          SMI_Push(&to_network, &val);

        }
      }
    }
    D(printf(ANSI_COLOR_RED "[CST - %d] completed iteration %d\n" ANSI_COLOR_RESET,my_rank,iteration++);)

  }
}

kernel void ConvertSendBottom(const int i_px, const int i_py,char my_rank) {
    int iteration=0;
  while (1) {
      D(printf(ANSI_COLOR_RED "[CSB - %d] starting iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration);)

    SMI_Channel to_network =
        SMI_Open_send_channel(Y_LOCAL, SMI_FLOAT, (i_px + 1) * PY + i_py, 2);
    VTYPE vec[B];
    #pragma loop_coalesce
    for (int i = 0; i < Y_LOCAL / (B * W); ++i) {
      for (int b0 = 0; b0 < B; ++b0) {
        for (int w = 0; w < W; ++w) {
          if (b0 == 0 && w == 0) {
            #pragma unroll
            for (int b1 = 0; b1 < B; ++b1) {
              vec[b1] = read_channel_intel(send_bottom[b1]);
            }
          }
          //mem_fence(CLK_GLOBAL_MEM_FENCE | CLK_CHANNEL_MEM_FENCE);

          float val = vec[b0][w];
         // mem_fence(CLK_CHANNEL_MEM_FENCE);

          SMI_Push(&to_network, &val);
        }
      }
    }
    D(printf(ANSI_COLOR_RED "[CSB - %d] completed iteration %d\n"ANSI_COLOR_RESET,my_rank,iteration);)

  }
}

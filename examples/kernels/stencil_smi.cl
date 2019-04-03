#include "stencil.h"
#include "smi.h"

#if PX * PY != RANK_COUNT
#error "Incompatible number of stencil processes and number of communication ranks."
#endif

channel VTYPE read_stream __attribute__((depth((Y/W)/PY)));
channel VTYPE write_stream __attribute__((depth((Y/W)/PY)));
channel VTYPE receive_top __attribute__((depth((Y/W)/PY)));
channel VTYPE receive_bottom __attribute__((depth((Y/W)/PY)));
channel HTYPE receive_left __attribute__((depth(X/PX)));
channel HTYPE receive_right __attribute__((depth(X/PX)));
channel VTYPE send_top __attribute__((depth((Y/W)/PY)));
channel VTYPE send_bottom __attribute__((depth((Y/W)/PY)));
channel HTYPE send_left __attribute__((depth(X/PX)));
channel HTYPE send_right __attribute__((depth(X/PX)));

kernel void Read(__global volatile const VTYPE memory[], const int i_px,
                 const int i_py, const int timesteps) {
  // Extra artificial timestep to send first boundaries
  for (int t = 0; t < timesteps + 1; ++t) {
    // Swap the timestep modulo to accommodate first artificial timestep
    const int offset = (t == 0 || t % 2 == 1) ? 0 : X_LOCAL * (Y_LOCAL / W);
    // +1 for each boundary
    for (int i = 0; i < X_LOCAL + 2 * HALO_X; ++i) {
      // +1 for each boundary
      for (int j = 0; j < (Y_LOCAL / W) + 2; ++j) {
        VTYPE res = -1000;
        // oob: out of bounds with respect to the local domain, i.e., not
        // necessarily the global domain.
        const bool oob_top = i < HALO_X;
        const bool oob_bottom = i >= X_LOCAL + HALO_X;
        // It's assumed that the horizontal halo fits in one vector, such that
        // HALO_Y <= W (hence, we use 1 here).
        const bool oob_left = j < 1;
        const bool oob_right = j >= (Y_LOCAL / W) + 1;
        const bool valid_read =
            !oob_top && !oob_bottom && !oob_left && !oob_right;
        if (valid_read) {
          res = memory[offset + (i - HALO_X) * (Y_LOCAL / W) + j - 1];
        } else {
          // We don't need to communicate on the corner, as this is not used by
          // the stencil.
          const bool on_corner =
              (oob_top && oob_left) || (oob_top && oob_right) ||
              (oob_bottom && oob_left) || (oob_bottom && oob_right);
          if (oob_top) {
            if (i_px > 0 && t > 0 && !on_corner) {
              // Read from channel above
              res = read_channel_intel(receive_top);
            }
          } else if (oob_bottom) {
            if (i_px < PX - 1 && t > 0 && !on_corner) {
              // Read from channel below
              res = read_channel_intel(receive_bottom);
            }
          } else if (oob_left) {
            if (i_py > 0 && t > 0 && !on_corner) {
              // Read from left channel
              HTYPE read_horizontal = read_channel_intel(receive_left);

              // Populate elements within boundary, leaving the rest
              // uninitialized/dummy values
#if HALO_Y > 1
              #pragma unroll
              for (int w = 0; w < HALO_Y; ++w) {
                res[W - HALO_Y + w] = read_horizontal[w];
              }
#else
              res[W - 1] = read_horizontal;
#endif
            }
          } else if (oob_right) {
            if (i_py < PY - 1 && t > 0 && !on_corner) {
              // Read from right channel
              HTYPE read_horizontal = read_channel_intel(receive_right);

              // Populate elements within boundary, leaving the rest
              // uninitialized/dummy values
#if HALO_Y > 1
              #pragma unroll
              for (int w = 0; w < HALO_Y; ++w) {
                res[w] = read_horizontal[w];
              }
#else
              res[0] = read_horizontal;
#endif
            }
          }
        } // !valid_read
        write_channel_intel(read_stream, res);
      }
    }
  }
}

kernel void Stencil(const int i_px, const int i_py, const int timesteps) {
  for (int t = 0; t < timesteps + 1; ++t) {
    DTYPE buffer[(2 * HALO_X) * (Y_LOCAL + 2 * W) + W];
    for (int i = 0; i < X_LOCAL + 2 * HALO_X; ++i) {
      for (int j = 0; j < (Y_LOCAL / W) + 2; ++j) {
        // Shift buffer
        #pragma unroll
        for (int b = 0; b < (2 * HALO_X) * (Y_LOCAL + 2 * W); ++b) {
          buffer[b] = buffer[b + W];
        }
        // Read into front
        VTYPE read = read_channel_intel(read_stream);
        #pragma unroll
        for (int w = 0; w < W; ++w) {
          buffer[(2 * HALO_X) * (Y_LOCAL + 2 * W) + w] = read[w];
        }
        // If in bounds, compute and output
        if (i >= 2 * HALO_X && j >= 1 && j < (Y_LOCAL / W) + 1) {
          VTYPE res;
          #pragma unroll
          for (int w = 0; w < W; ++w) {
            if ((i_px == 0 && i < 3 * HALO_X) ||
                (i_px == PX - 1 && i >= X_LOCAL + HALO_X) ||
                (i_py == 0 && j * W + w < W + HALO_Y) ||
                (i_py == PY - 1 && j * W + w >= W + Y_LOCAL - HALO_Y) ||
                t == 0) {
              // Just forward value if on the boundary, or if on the first
              // artifical timestep
              res[w] = buffer[Y_LOCAL + 2 * W + w];
            } else {
              res[w] = 0.25 * (buffer[2 * (Y_LOCAL + 2 * W) + w] +  // South
                               buffer[Y_LOCAL + 2 * W + w - 1] +    // West
                               buffer[Y_LOCAL + 2 * W + w + 1] +    // East
                               buffer[w]);                          // North
            }
          }
          write_channel_intel(write_stream, res);
        }
      }
    }
  }
}

kernel void Write(__global volatile VTYPE memory[], const int i_px,
                  const int i_py, const int timesteps) {
  // Extra timestep to write first halos before starting computation
  for (int t = 0; t < timesteps + 1; ++t) {
    // Extra artifical timestep shifts the offset
    int offset = (t % 2 == 0) ? 0 : X_LOCAL * (Y_LOCAL / W);
    for (int i = 0; i < X_LOCAL; ++i) {
      for (int j = 0; j < (Y_LOCAL / W); ++j) {
        VTYPE read = read_channel_intel(write_stream);
        if (i_px > 0 && i < HALO_X) {
          if (t < timesteps) { // Don't communicate on last timestep
            // Write to channel above
            write_channel_intel(send_top, read);
          }
        }
        if (i_px < PX - 1 && i >= X_LOCAL - HALO_X) {
          if (t < timesteps) { // Don't communicate on last timestep
            // Write channel below
            write_channel_intel(send_bottom, read);
          }
        }
        if (i_py > 0 && j < 1) {
          if (t < timesteps) { // Don't communicate on last timestep
            // Extract relevant values
#if HALO_Y > 1
            HTYPE write_horizontal = -1000;
            #pragma unroll
            for (int w = 0; w < HALO_Y; ++w) {
              write_horizontal[w] = read[w];
            }
#else
            HTYPE write_horizontal = read[0];
#endif
            // Write to left channel
            write_channel_intel(send_left, write_horizontal);
          }
        }
        if (i_py < PY - 1 && j >= (Y_LOCAL / W) - 1) {
          if (t < timesteps) {
            // Extract relevant values
#if HALO_Y > 1
            HTYPE write_horizontal = -1000;
            #pragma unroll
            for (int w = 0; w < HALO_Y; ++w) {
              write_horizontal[w] = read[W - HALO_Y + w];
            }
#else
            HTYPE write_horizontal = read[W - 1];
#endif
            // Write to right channel
            write_channel_intel(send_right, write_horizontal);
          }
        }
        if (t > 0) {
          memory[offset + i * (Y_LOCAL / W) + j] = read;
        }
      }
    }
  }
}

kernel void ConvertReceiveLeft(const int i_px, const int i_py) {
  while (1) {
#if HALO_Y > 1
    #error "NYI"
#endif
    SMI_Channel from_network =
        SMI_Open_receive_channel(X_LOCAL, SMI_TYPE, i_px * PY + (i_py - 1), 1);
    for (int i = 0; i < X_LOCAL; ++i) {
      DTYPE val; 
      SMI_Pop(&from_network, &val);
      write_channel_intel(receive_left, val);
    }
  }
}

kernel void ConvertReceiveRight(const int i_px, const int i_py) {
  while (1) {
#if HALO_Y > 1
    #error "NYI"
#endif
    SMI_Channel from_network =
        SMI_Open_receive_channel(X_LOCAL, SMI_TYPE, i_px * PY + (i_py + 1), 3);
    for (int i = 0; i < X_LOCAL; ++i) {
      DTYPE val; 
      SMI_Pop(&from_network, &val);
      write_channel_intel(receive_right, val);
    }
  }
}

kernel void ConvertReceiveTop(const int i_px, const int i_py) {
  while (1) {
    SMI_Channel from_network =
        SMI_Open_receive_channel(Y_LOCAL, SMI_TYPE, (i_px - 1) * PY + i_py, 2);
    #pragma loop_coalesce
    for (int i = 0; i < Y_LOCAL / W; ++i) {
      VTYPE vec;
      for (int w = 0; w < W; ++w) {
        DTYPE val;
        SMI_Pop(&from_network, &val);
        vec[w] = val;
      }
      write_channel_intel(receive_top, vec);
    }
  }
}

kernel void ConvertReceiveBottom(const int i_px, const int i_py) {
  while (1) {
    SMI_Channel from_network =
        SMI_Open_receive_channel(Y_LOCAL, SMI_TYPE, (i_px + 1) * PY + i_py, 0);
    #pragma loop_coalesce
    for (int i = 0; i < Y_LOCAL / W; ++i) {
      VTYPE vec;
      for (int w = 0; w < W; ++w) {
        DTYPE val;
        SMI_Pop(&from_network, &val);
        vec[w] = val;
      }
      write_channel_intel(receive_bottom, vec);
    }
  }
}

kernel void ConvertSendLeft(const int i_px, const int i_py) {
  while (1) {
#if HALO_Y > 1
    #error "NYI"
#endif
    SMI_Channel to_network =
        SMI_Open_send_channel(X_LOCAL, SMI_TYPE, i_px * PY + i_py - 1, 3);
    for (int i = 0; i < X_LOCAL; ++i) {
      DTYPE val = read_channel_intel(send_left);
      SMI_Push(&to_network, &val);
    }
  }
}

kernel void ConvertSendRight(const int i_px, const int i_py) {
  while (1) {
#if HALO_Y > 1
    #error "NYI"
#endif
    SMI_Channel to_network =
        SMI_Open_send_channel(X_LOCAL, SMI_TYPE, i_px * PY + i_py + 1, 1);
    for (int i = 0; i < X_LOCAL; ++i) {
      DTYPE val = read_channel_intel(send_right);
      SMI_Push(&to_network, &val);
    }
  }
}

kernel void ConvertSendTop(const int i_px, const int i_py) {
  while (1) {
    SMI_Channel to_network =
        SMI_Open_receive_channel(Y_LOCAL, SMI_TYPE, (i_px - 1) * PY + i_py, 0);
    #pragma loop_coalesce
    for (int i = 0; i < Y_LOCAL / W; ++i) {
      VTYPE vec = read_channel_intel(send_top);
      for (int w = 0; w < W; ++w) {
        DTYPE val = vec[w];
        SMI_Push(&to_network, &val);
      }
    }
  }
}

kernel void ConvertSendBottom(const int i_px, const int i_py) {
  while (1) {
    SMI_Channel to_network =
        SMI_Open_receive_channel(Y_LOCAL, SMI_TYPE, (i_px + 1) * PY + i_py, 2);
    #pragma loop_coalesce
    for (int i = 0; i < Y_LOCAL / W; ++i) {
      VTYPE vec = read_channel_intel(send_bottom);
      for (int w = 0; w < W; ++w) {
        DTYPE val = vec[w];
        SMI_Push(&to_network, &val);
      }
    }
  }
}

#include "hotspot.h"
#include "smi.h"

#if PX * PY != RANK_COUNT
#error "Incompatible number of stencil processes and number of communication ranks."
#endif

channel VTYPE read_stream[B] __attribute__((depth((Y/(W*B))/PY)));
channel VTYPE power_stream[B] __attribute__((depth((Y/(W*B))/PY)));
channel VTYPE write_stream[B] __attribute__((depth((Y/(W*B))/PY)));
channel VTYPE receive_top[B] __attribute__((depth((Y/(W*B))/PY)));
channel VTYPE receive_bottom[B] __attribute__((depth((Y/(W*B))/PY)));
channel float receive_left __attribute__((depth(X/PX)));
channel float receive_right __attribute__((depth(X/PX)));
channel VTYPE send_top[B] __attribute__((depth((Y/(W*B))/PY)));
channel VTYPE send_bottom[B] __attribute__((depth((Y/(W*B))/PY)));
channel float send_left __attribute__((depth(X/PX)));
channel float send_right __attribute__((depth(X/PX)));

kernel void Read(__global volatile const VTYPE *restrict bank0,
                 __global volatile const VTYPE *restrict bank1,
                 __global volatile const VTYPE *restrict bank2,
                 __global volatile const VTYPE *restrict bank3, const int i_px,
                 const int i_py, const int timesteps) {
  // Extra artificial timestep to send first boundaries
  for (int t = 0; t < timesteps + 1; ++t) {
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
              #pragma unroll
              for (int b = 0; b < B; ++b) {
                res[b] = read_channel_intel(receive_top[b]);
              }
            }
          } else if (oob_bottom) {
            if (i_px < PX - 1 && t > 0 && !on_corner) {
              // Read from channel below
              #pragma unroll
              for (int b = 0; b < B; ++b) {
                res[b] = read_channel_intel(receive_bottom[b]);
              }
            }
          } else if (oob_left) {
            if (i_py > 0 && t > 0 && !on_corner) {
              // Read from left channel
              float read_horizontal = read_channel_intel(receive_left);

              // Populate elements within boundary, leaving the rest
              // uninitialized/dummy values
              res[B - 1][W - 1] = read_horizontal;
            }
          } else if (oob_right) {
            if (i_py < PY - 1 && t > 0 && !on_corner) {
              // Read from right channel
              float read_horizontal = read_channel_intel(receive_right);

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
      }
    }
  }
}

kernel void ReadPower(__global volatile const VTYPE *restrict bank0,
                      __global volatile const VTYPE *restrict bank1,
                      __global volatile const VTYPE *restrict bank2,
                      __global volatile const VTYPE *restrict bank3,
                      const int timesteps) {
  // Extra artificial timestep to send first boundaries
  for (int t = 0; t < timesteps + 1; ++t) {
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
        }
        #pragma unroll
        for (int b = 0; b < B; ++b) {
          write_channel_intel(power_stream[b], res[b]);
        }
      }
    }
  }
}

kernel void Stencil(const float rx1, const float ry1, const float rz1,
                    const float sdc, const int i_px, const int i_py,
                    const int timesteps) {
  for (int t = 0; t < timesteps + 1; ++t) {
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
              }
            }
            write_channel_intel(write_stream[b], res);
          }
        }
      }
    }
  }
}

kernel void Write(__global volatile VTYPE *restrict bank0,
                  __global volatile VTYPE *restrict bank1,
                  __global volatile VTYPE *restrict bank2,
                  __global volatile VTYPE *restrict bank3, const int i_px,
                  const int i_py, const int timesteps) {
  // Extra timestep to write first halos before starting computation
  for (int t = 0; t < timesteps + 1; ++t) {
    // Extra artifical timestep shifts the offset
    int offset = (t % 2 == 0) ? 0 : X_LOCAL * (Y_LOCAL / (B * W));
    for (int i = 0; i < X_LOCAL; ++i) {
      for (int j = 0; j < Y_LOCAL / (B * W); ++j) {
        VTYPE read[B];
        #pragma unroll
        for (int b = 0; b < B; ++b) {
          read[b] = read_channel_intel(write_stream[b]);
        }
        if (i_px > 0 && i < 1) {
          if (t < timesteps) { // Don't communicate on last timestep
            // Write to channel above
            #pragma unroll
            for (int b = 0; b < B; ++b) {
              write_channel_intel(send_top[b], read[b]);
            }
          }
        }
        if (i_px < PX - 1 && i >= X_LOCAL - 1) {
          if (t < timesteps) { // Don't communicate on last timestep
            // Write channel below
            #pragma unroll
            for (int b = 0; b < B; ++b) {
              write_channel_intel(send_bottom[b], read[b]);
            }
          }
        }
        if (i_py > 0 && j < 1) {
          if (t < timesteps) { // Don't communicate on last timestep
            // Extract relevant values
            float write_horizontal = read[0][0];
            // Write to left channel
            write_channel_intel(send_left, write_horizontal);
          }
        }
        if (i_py < PY - 1 && j >= (Y_LOCAL / (B * W)) - 1) {
          if (t < timesteps) {
            // Extract relevant values
            float write_horizontal = read[B - 1][W - 1];
            // Write to right channel
            write_channel_intel(send_right, write_horizontal);
          }
        }
        if (t > 0) {
          bank0[offset + i * (Y_LOCAL / (B * W)) + j] = read[0];
          bank1[offset + i * (Y_LOCAL / (B * W)) + j] = read[1];
          bank2[offset + i * (Y_LOCAL / (B * W)) + j] = read[2];
          bank3[offset + i * (Y_LOCAL / (B * W)) + j] = read[3];
        }
      }
    }
  }
}

kernel void ConvertReceiveLeft(const int i_px, const int i_py) {
  while (1) {
    SMI_Channel from_network =
        SMI_Open_receive_channel(X_LOCAL, SMI_FLOAT, i_px * PY + (i_py - 1), 1);
    for (int i = 0; i < X_LOCAL; ++i) {
      float val; 
      SMI_Pop(&from_network, &val);
      write_channel_intel(receive_left, val);
    }
  }
}

kernel void ConvertReceiveRight(const int i_px, const int i_py) {
  while (1) {
    SMI_Channel from_network =
        SMI_Open_receive_channel(X_LOCAL, SMI_FLOAT, i_px * PY + (i_py + 1), 3);
    for (int i = 0; i < X_LOCAL; ++i) {
      float val; 
      SMI_Pop(&from_network, &val);
      write_channel_intel(receive_right, val);
    }
  }
}

kernel void ConvertReceiveTop(const int i_px, const int i_py) {
  while (1) {
    SMI_Channel from_network =
        SMI_Open_receive_channel(Y_LOCAL, SMI_FLOAT, (i_px - 1) * PY + i_py, 2);
    VTYPE vec[B];
    #pragma loop_coalesce
    for (int i = 0; i < Y_LOCAL / (B * W); ++i) {
      for (int b0 = 0; b0 < B; ++b0) {
        for (int w = 0; w < W; ++w) {
          float val;
          SMI_Pop(&from_network, &val);
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
  }
}

kernel void ConvertReceiveBottom(const int i_px, const int i_py) {
  while (1) {
    SMI_Channel from_network =
        SMI_Open_receive_channel(Y_LOCAL, SMI_FLOAT, (i_px + 1) * PY + i_py, 0);
    VTYPE vec[B];
    #pragma loop_coalesce
    for (int i = 0; i < Y_LOCAL / (B * W); ++i) {
      for (int b0 = 0; b0 < B; ++b0) {
        for (int w = 0; w < W; ++w) {
          float val;
          SMI_Pop(&from_network, &val);
          vec[b0][w] = val;
          if (b0 == B - 1 && w == W - 1) {
            #pragma unroll
            for (int b1 = 0; b1 < B; ++b1) {
              write_channel_intel(receive_bottom[b1], vec[b1]);
            }
          }
        }
      }
    }
  }
}

kernel void ConvertSendLeft(const int i_px, const int i_py) {
  while (1) {
    SMI_Channel to_network =
        SMI_Open_send_channel(X_LOCAL, SMI_FLOAT, i_px * PY + i_py - 1, 3);
    for (int i = 0; i < X_LOCAL; ++i) {
      float val = read_channel_intel(send_left);
      SMI_Push(&to_network, &val);
    }
  }
}

kernel void ConvertSendRight(const int i_px, const int i_py) {
  while (1) {
    SMI_Channel to_network =
        SMI_Open_send_channel(X_LOCAL, SMI_FLOAT, i_px * PY + i_py + 1, 1);
    for (int i = 0; i < X_LOCAL; ++i) {
      float val = read_channel_intel(send_right);
      SMI_Push(&to_network, &val);
    }
  }
}

kernel void ConvertSendTop(const int i_px, const int i_py) {
  while (1) {
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
          float val = vec[b0][w];
          SMI_Push(&to_network, &val);
        }
      }
    }
  }
}

kernel void ConvertSendBottom(const int i_px, const int i_py) {
  while (1) {
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
          float val = vec[b0][w];
          SMI_Push(&to_network, &val);
        }
      }
    }
  }
}

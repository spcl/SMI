
#pragma OPENCL EXTENSION cl_intel_channels : enable

#include "../host/k_means_routing/smi.h"
#include "../include/kmeans_new.h"

channel VTYPE centroid_ch __attribute__((depth(2 * K * DIMS / W)));
channel VTYPE centroid_loop_ch __attribute__((depth(2 * K * DIMS / W)));

kernel void SendCentroids(__global volatile const VTYPE centroids_global[],
                          const int iterations, const int smi_rank,
                          const int smi_size) {

  printf("[%d] SendCentroids\n",smi_rank);
  //#pragma loop_coalesce TODO
  for (int i = 0; i < iterations; ++i) {
    // printf("[%i] SendCentroids iteration %i\n", smi_rank, i);
    for (int k = 0; k < K; ++k) {
      for (int d = 0; d < DIMS / W; ++d) {
        VTYPE val;
        if (i == 0) {
          // On first iteration, read centroids from memory
          val = centroids_global[k * DIMS / W + d];
        } else {
          // On following iterations, read centroids from global memory
          val = read_channel_intel(centroid_loop_ch);
        }
       // write_channel_intel(centroid_ch, val); TODO
      }
    }
  }
}

__kernel void ComputeMeans(__global volatile VTYPE centroids_global[],
                           const int num_points, const int iterations,
                           const int smi_rank, const int smi_size)
{

    printf("[%d] ComputeMeans\n",smi_rank);
    for (int i = 0; i < iterations; ++i)
    {

        // Compute mean for new centroids while iterating
        VTYPE means[DIMS / W][K];
        for (int d = 0; d < DIMS / W; ++d) {
          #pragma unroll
          for (int k = 0; k < K; ++k) {
            means[d][k] = 0;
          }
        }
        int count[K];
        #pragma unroll
        for (int k = 0; k < K; ++k) {
          count[k] = 0;
        }

        for (int p = 0; p < num_points; ++p) {  // Pipelined
          ITYPE index ;//= read_channel_intel(index_ch);TODO
          // These loop are not flattened, as the compiler fails to do accumulation
          // otherwise. This means that the inner loop will fully drain before the
          // next iteration, making this slow for small dimensionality.
          for (int d = 0; d < DIMS / W; ++d) {  // Pipelined
            VTYPE dims; /*= read_channel_intel(dims_ch); TODO*/
            #pragma unroll
            for (int k = 0; k < K; ++k) {
              means[d][k] += (index == k) ? dims : 0;
              count[k] += (index == k) ? 1 : 0;
            }
          }
        }

        DTYPE means_reduced[W][DIMS / W][K];


        SMI_RChannel  __attribute__((register)) reduce_mean_ch= SMI_Open_reduce_channel(K*DIMS, SMI_FLOAT, 0,smi_rank,smi_size);
        //#pragma loop_coalesce //TODO
        for (int k = 0; k < K; ++k) {
          for (int d = 0; d < DIMS / W; d++) {
            VTYPE send_vec = means[d][k];
            VTYPE recv_vec;
            for (int w = 0; w < W; ++w) {
#if W>1
              DTYPE send_val = send_vec[w];
#else
              DTYPE send_val=send_vec;
#endif
              DTYPE recv_val;
              SMI_Reduce_float(&reduce_mean_ch, &send_val, &recv_val);
              // It doesn't matter that we write junk on non-0 ranks
              means_reduced[w][d][k] = recv_val;
            }
          }
        }

        VTYPE centroids_updated[DIMS / W][K];

        SMI_BChannel  __attribute__((register)) broadcast_mean_ch=  SMI_Open_bcast_channel(K*DIMS, SMI_FLOAT, 0,smi_rank,smi_size);

        //#pragma loop_coalesce TODO
        for (int k = 0; k < K; ++k) {
          for (int d = 0; d < DIMS / W; d++) {
            VTYPE bcast_vec;
            for (int w = 0; w < W; ++w) {
              DTYPE bcast_val = means_reduced[w][d][k];
              SMI_Bcast_float(&broadcast_mean_ch, &bcast_val);
#if W>1
              bcast_vec[w] = bcast_val;
#else
              bcast_vec=bcast_val;
#endif
            }
            centroids_updated[d][k] = bcast_vec;
          }
        }

        int count_reduced[K];

        SMI_RChannel  __attribute__((register)) reduce_count_ch= SMI_Open_reduce_channel(K, SMI_INT, 0,smi_rank,smi_size);
        for (int k = 0; k < K; k++) {
          int send_val = count[k];
          int recv_val;
          SMI_Reduce_int(&reduce_count_ch, &send_val, &recv_val);
          // Doesn't matter that this is junk on non-root ranks
          count_reduced[k] = recv_val;
        }

        int count_updated[K];

        SMI_BChannel  __attribute__((register)) broadcast_count_ch= SMI_Open_bcast_channel(K, SMI_INT, 0,smi_rank,smi_size);

        for (int k = 0; k < K; k++) {
          int bcast_val = count_reduced[k];
          SMI_Bcast_int(&broadcast_count_ch, &bcast_val);
          count_updated[k] = bcast_val;
        }

        // Compute means by dividing by count
        #pragma loop_coalesce
        for (int k = 0; k < K; ++k) {
          for (int d = 0; d < DIMS / W; ++d) {
            VTYPE updated = centroids_updated[d][k] / count_updated[k];
            write_channel_intel(centroid_loop_ch, updated);
            // Write back to global memory
            centroids_global[k * DIMS / W + d] = updated;
          }
        }



    }
}

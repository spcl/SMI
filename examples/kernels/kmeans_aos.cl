#include "kmeans.h"

// Every channel is set to twice the size it needs to be
channel VTYPE dims_ch __attribute__((depth(2 * DIMS / W)));
channel ITYPE index_ch __attribute__((depth(2)));
channel VTYPE centroid_ch __attribute__((depth(2 * K * DIMS / W)));
channel VTYPE centroid_loop_ch __attribute__((depth(2 * K * DIMS / W)));

kernel void SendCentroids(__global volatile const VTYPE centroids_global[],
                          const int iterations, const int smi_rank,
                          const int smi_size) {
  #pragma loop_coalesce
  for (int i = 0; i < iterations; ++i) {
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
        write_channel_intel(centroid_ch, val);
      }
    }
  }
}

kernel void ComputeDistance(__global volatile const VTYPE points[],
                            const int num_points, const int iterations,
                            const int smi_rank, const int smi_size) {

  for (int i = 0; i < iterations; ++i) {

    VTYPE centroids[DIMS / W][K];

    // Load centroids into local memory
    #pragma loop_coalesce
    for (int k = 0; k < K; ++k) {  // Pipelined
      for (int d = 0; d < DIMS / W; d++) {  // Pipelined
        centroids[d][k] = read_channel_intel(centroid_ch);
      }
    }

    for (int p = 0; p < num_points; ++p) {
      DTYPE dist[K];
      #pragma unroll
      for (int k = 0; k < K; ++k) {
        dist[k] = 0;
      }
      for (int d = 0; d < DIMS / W; ++d) {  // Pipelined
        VTYPE x = points[p * DIMS / W + d];
        write_channel_intel(dims_ch, x);  // Forward to mean kernel
        #pragma unroll
        for (int k = 0; k < K; ++k) {
          DTYPE dist_contribution = 0;
          VTYPE centroid = centroids[d][k];
          #pragma unroll
          for (int w = 0; w < W; ++w) {
            DTYPE diff = x[w] - centroid[w]; 
            dist_contribution = diff * diff; 
          }
          dist[k] += dist_contribution;
        }
      }
      DTYPE min_dist = INFINITY;
      unsigned min_idx = 0;
      #pragma unroll
      for (int k = 0; k < K; ++k) {
        if (dist[k] < min_dist) {
          min_dist = dist[k];
          min_idx = k;
        }
      }
      write_channel_intel(index_ch, min_idx);
    }

  }
}

kernel void ComputeMeans(__global volatile VTYPE centroids_global[],
                         const int num_points, const int iterations,
                         const int smi_rank, const int smi_size) {

  for (int i = 0; i < iterations; ++i) {

    // Compute mean for new centroids while iterating
    VTYPE means[DIMS / W][K];
    for (int d = 0; d < DIMS / W; ++d) {
      #pragma unroll
      for (int k = 0; k < K; ++k) {
        means[d][k] = 0;
      }
    }
    unsigned count[K];
    #pragma unroll
    for (int k = 0; k < K; ++k) {
      count[k] = 0;
    }

    for (int p = 0; p < num_points; ++p) {  // Pipelined
      ITYPE index = read_channel_intel(index_ch);
      // These loop are not flattened, as the compiler fails to do accumulation
      // otherwise. This means that the inner loop will fully drain before the
      // next iteration, making this slow for small dimensionality.
      for (int d = 0; d < DIMS / W; ++d) {  // Pipelined
        VTYPE dims = read_channel_intel(dims_ch);
        #pragma unroll
        for (int k = 0; k < K; ++k) {
          means[d][k] += (index == k) ? dims : 0;
          count[k] += (index == k) ? 1 : 0;
        }
      }
    }

    // Compute means by dividing by count
    #pragma loop_coalesce
    for (int k = 0; k < K; ++k) {
      for (int d = 0; d < DIMS / W; ++d) {
        VTYPE updated = means[d][k] / count[k]; 
        write_channel_intel(centroid_loop_ch, updated);
        // Write back to global memory
        centroids_global[k * DIMS / W + d] = updated; 
      }
    }

  }
}

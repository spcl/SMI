#include "kmeans.h"
#include "smi.h"

kernel void KMeans(__global volatile VTYPE centroids_global[],
                   __global volatile const VTYPE points_global[],
                   const int num_points, const int iterations,
                   const int smi_rank, const int smi_size) {

  VTYPE centroids[DIMS / W][K];

  // Load initial centroids from global memory 
  #pragma loop_coalesce
  for (int k = 0; k < K; ++k) {  // Pipelined
    for (int d = 0; d < DIMS / W; d++) {  // Pipelined
      centroids[d][k] = centroids_global[k * DIMS / W + d];
    }
  }

  #pragma unroll 1
  #pragma max_concurrency 1
  for (int i = 0; i < iterations; ++i) {

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

    #pragma unroll 1
    #pragma max_concurrency 1
    for (int p = 0; p < num_points; ++p) {
      DTYPE dist[K];
      #pragma unroll
      for (int k = 0; k < K; ++k) {
        dist[k] = 0;
      }
      VTYPE point_local[DIMS / W];
      #pragma unroll 1
      #pragma max_concurrency 1
      for (int d = 0; d < DIMS / W; ++d) {  // Pipelined
        VTYPE x = points_global[p * DIMS / W + d];
        point_local[d] = x;
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
      int min_idx = 0;
      #pragma unroll
      for (int k = 0; k < K; ++k) {
        if (dist[k] < min_dist) {
          min_dist = dist[k];
          min_idx = k;
        }
      }
      #pragma unroll 1
      #pragma max_concurrency 1
      for (int d = 0; d < DIMS / W; ++d) {  // Pipelined
        VTYPE x = point_local[d];
        #pragma unroll
        for (int k = 0; k < K; ++k) {
          means[d][k] += (min_idx == k) ? x : 0;
          count[k] += (min_idx == k) ? 1 : 0;
        }
      }
    }

    DTYPE means_reduced[W][DIMS / W][K];

    // printf("[%i] Starting centroid reduce...\n", smi_rank);

    SMI_RChannel __attribute__((register)) reduce_mean_ch =
        SMI_Open_reduce_channel(K * DIMS, SMI_FLOAT, 0, smi_rank, smi_size);
    #pragma loop_coalesce
    for (int k = 0; k < K; ++k) {
      for (int d = 0; d < DIMS / W; d++) {
        VTYPE send_vec = means[d][k];
        VTYPE recv_vec;
        for (int w = 0; w < W; ++w) {
          DTYPE send_val = send_vec[w];
          DTYPE recv_val;
          SMI_Reduce_float(&reduce_mean_ch, &send_val, &recv_val);
          // It doesn't matter that we write junk on non-0 ranks
          means_reduced[w][d][k] = recv_val;
        }
      }
    }

    VTYPE centroids_updated[DIMS / W][K];

    // printf("[%i] Starting centroid broadcast...\n", smi_rank);

    SMI_BChannel __attribute__((register)) broadcast_mean_ch =
        SMI_Open_bcast_channel(K * DIMS, SMI_FLOAT, 0, smi_rank, smi_size);
    #pragma loop_coalesce
    for (int k = 0; k < K; ++k) {
      for (int d = 0; d < DIMS / W; d++) {
        VTYPE bcast_vec;
        for (int w = 0; w < W; ++w) {
          DTYPE bcast_val = means_reduced[w][d][k];
          SMI_Bcast_float(&broadcast_mean_ch, &bcast_val);
          bcast_vec[w] = bcast_val;
        }
        centroids_updated[d][k] = bcast_vec;
      }
    }

    int count_reduced[K];

    // printf("[%i] Starting count reduce...\n", smi_rank);

    SMI_RChannel __attribute__((register)) reduce_count_ch =
        SMI_Open_reduce_channel(K, SMI_INT, 0, smi_rank, smi_size);
    for (int k = 0; k < K; k++) {
      int send_val = count[k]; 
      int recv_val;
      SMI_Reduce_int(&reduce_count_ch, &send_val, &recv_val);
      // Doesn't matter that this is junk on non-root ranks
      count_reduced[k] = recv_val;
    }

    int count_updated[K];

    // printf("[%i] Starting count broadcast...\n", smi_rank);

    SMI_BChannel __attribute__((register)) broadcast_count_ch =
        SMI_Open_bcast_channel(K, SMI_INT, 0, smi_rank, smi_size);
    for (int k = 0; k < K; k++) {
      int bcast_val = count_reduced[k];
      SMI_Bcast_int(&broadcast_count_ch, &bcast_val);
      count_updated[k] = bcast_val;
    }

    // printf("[%i] Writing back centroids...\n", smi_rank);

    // Compute means by dividing by count
    #pragma loop_coalesce
    for (int k = 0; k < K; ++k) {
      for (int d = 0; d < DIMS / W; ++d) {
        VTYPE updated = centroids_updated[d][k] / count_updated[k]; 
        // Write back to global memory
        centroids[d][k] = updated; 
      }
    }

  }

  // Store final centroids into memory 
  #pragma loop_coalesce
  for (int k = 0; k < K; ++k) {  // Pipelined
    for (int d = 0; d < DIMS / W; d++) {  // Pipelined
       centroids_global[k * DIMS / W + d] = centroids[d][k];
    }
  }
}

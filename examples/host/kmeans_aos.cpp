#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include "hlslib/intel/OpenCL.h"
#include "kmeans.h"

// Convert from C to C++
using Data_t = DTYPE;
constexpr int kK = K;
constexpr int kDims = DIMS;
constexpr auto kUsage =
    "Usage: ./kmeans_aos <[emulator/hardware]> <num_points> <iterations>\n";

using AlignedVec_t =
    std::vector<Data_t, hlslib::ocl::AlignedAllocator<Data_t, 64>>;

int main(int argc, char **argv) {
  // Handle input arguments
  if (argc != 4) {
    std::cout << kUsage;
    return 1;
  }
  bool emulator = false;
  std::string mode_str(argv[1]);
  std::string kernel_path;
  if (mode_str == "emulator") {
    emulator = true;
    setenv("CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA", "1", false);
    kernel_path = "kmeans_aos_emulator.aocx";
  } else if (mode_str == "hardware") {
    kernel_path = "kmeans_aos_hardware.aocx";
    emulator = false;
  } else {
    std::cout << kUsage;
    return 2;
  }

  const int num_points = std::stoi(argv[2]);
  const int iterations = std::stoi(argv[3]);

  std::cout << "Initializing host memory...\n" << std::flush;
  // std::random_device rd;
  std::default_random_engine rng(5);
  AlignedVec_t points(num_points*kDims);  // TODO: load some data set
  // Generate Gaussian means with a uniform distribution
  std::uniform_real_distribution<Data_t> dist_means(-5, 5);
  AlignedVec_t gaussian_means(kK*kDims);
  // Randomize centers for Gaussian distributions
  for (int k = 0; k < kK; ++k) {
    for (int d = 0; d < kDims; ++d) {
      gaussian_means[k * kDims + d] = dist_means(rng);
    }
  }
  // Print source means 
  std::cout << "Means used to generate data:\n";
  for (int k = 0; k < kK; ++k) {
    std::cout << "  {" << gaussian_means[k * kDims];
    for (int d = 1; d < kDims; ++d) {
      std::cout << ", " << gaussian_means[k * kDims + d];
    }
    std::cout << "}\n";
  }
  // Generate data with a separate Gaussian at each mean 
  std::normal_distribution<Data_t> normal_dist;
  for (int k = 0; k < kK; ++k) {
    const int n_per_centroid = num_points / kK;
    for (int i = k * n_per_centroid; i < (k + 1) * n_per_centroid; ++i) {
      for (int d = 0; d < kDims; ++d) {
        points[i * kDims + d] =
            normal_dist(rng) + gaussian_means[k * kDims + d];
      }
    }
  }
  // For sampling indices for starting centroids
  std::uniform_int_distribution<size_t> index_dist(0, num_points);
  AlignedVec_t centroids(kK * kDims);
  // Choose initial centroids
  for (int k = 0; k < kK; ++k) {
    auto i = index_dist(rng);
    std::copy(&points[i * kDims], &points[i * kDims + kDims],
              &centroids[k * kDims]);
  }

  // Print starting centroids 
  std::cout << "Initial centroids:\n";
  for (int k = 0; k < kK; ++k) {
    std::cout << "  {" << centroids[k * kDims];
    for (int d = 1; d < kDims; ++d) {
      std::cout << ", " << centroids[k * kDims + d];
    }
    std::cout << "}\n";
  }

  // Create OpenCL kernels
  std::cout << "Creating OpenCL context...\n" << std::flush;
  hlslib::ocl::Context context;
  std::cout << "Allocating device memory...\n" << std::flush;
  auto points_device = context.MakeBuffer<Data_t, hlslib::ocl::Access::read>(
      points.cbegin(), points.cend());
  auto centroids_device_read =
      context.MakeBuffer<Data_t, hlslib::ocl::Access::read>(
          centroids.cbegin(), centroids.cend());
  auto centroids_device_write =
      context.MakeBuffer<Data_t, hlslib::ocl::Access::write>(
          centroids.cbegin(), centroids.cend());
  std::cout << "Creating program from binary...\n" << std::flush;
  auto program = context.MakeProgram(kernel_path);
  std::cout << "Creating kernels...\n" << std::flush;
  std::vector<hlslib::ocl::Kernel> kernels;
  kernels.emplace_back(program.MakeKernel(
      "SendCentroids", centroids_device_read, iterations, 0, 1));
  kernels.emplace_back(program.MakeKernel("ComputeDistance", points_device,
                                          num_points, iterations, 0, 1));
  kernels.emplace_back(program.MakeKernel(
      "ComputeMeans", centroids_device_write, num_points, iterations, 0, 1));
  std::cout << "Copying data to device...\n" << std::flush;

  // Execute kernel
  std::cout << "Launching kernels...\n" << std::flush;
  std::vector<std::future<std::pair<double, double>>> futures;
  const auto start = std::chrono::high_resolution_clock::now();
  for (auto &k : kernels) {
    futures.emplace_back(k.ExecuteTaskAsync());
  }
  std::cout << "Waiting for kernels to finish...\n" << std::flush;
  for (auto &f : futures) {
    f.wait();
  }
  const auto end = std::chrono::high_resolution_clock::now();
  const double elapsed =
      1e-9 *
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  std::cout << "Finished in " << elapsed << " seconds.\n" << std::flush;

  // Copy back result
  std::cout << "Copying back result...\n" << std::flush;
  centroids_device_write.CopyToHost(centroids.begin());

  // Final centroids 
  std::cout << "Final centroids:\n";
  for (int k = 0; k < kK; ++k) {
    std::cout << "  {" << centroids[k * kDims];
    for (int d = 1; d < kDims; ++d) {
      std::cout << ", " << centroids[k * kDims + d];
    }
    std::cout << "}\n";
  }


  return 0;
}

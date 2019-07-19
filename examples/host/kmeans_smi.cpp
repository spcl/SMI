#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include "hlslib/intel/OpenCL.h"
#include "kmeans.h"
#include "common.h"
#define __HOST_PROGRAM__

#include <smi/communicator.h>
#include <mpi.h>

// Convert from C to C++
using Data_t = DTYPE;
constexpr int kNumTags = 4;
constexpr int kK = K;
constexpr int kDims = DIMS;
constexpr auto kUsage =
    "Usage: ./kmeans_smi <[emulator/hardware]> <num_points> <iterations>\n";
constexpr auto kDevicesPerNode = SMI_DEVICES_PER_NODE;

using AlignedVec_t =
    std::vector<Data_t, hlslib::ocl::AlignedAllocator<Data_t, 64>>;

void _MPIStatus(std::stringstream &) {}

template <typename T, typename... Ts>
void _MPIStatus(std::stringstream &ss, T &&arg, Ts &&... args) {
  ss << arg;
  _MPIStatus(ss, args...);
}

template <typename T, typename... Ts>
void MPIStatus(int rank, T &&arg, Ts... args) {
  std::stringstream ss;
  ss << "[" << rank << "] " << arg;
  _MPIStatus(ss, args...);
  std::cout << ss.str() << std::flush;
}

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);

  int mpi_size, mpi_rank;
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

  // Handle input arguments
  if (argc != 4) {
    std::cout << kUsage;
    return 1;
  }
  bool emulator = false;
  std::string mode_str(argv[1]);
  std::string kernel_path;
  if (mode_str == "emulator") {
    setenv("CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA", "1", false);
    emulator = true;
    // In emulation mode, each rank has its own kernel file
    kernel_path =
        ("emulator_" + std::to_string(mpi_rank) + "/kmeans_smi.aocx");

  } else if (mode_str == "hardware") {
    kernel_path = "kmeans_smi_hardware.aocx";
    emulator = false;
  } else {
    std::cout << kUsage;
    return 2;
  }

  const int num_points = std::stoi(argv[2]);
  const int points_per_rank = num_points / mpi_size;
  if (num_points % mpi_size != 0) {
    std::cerr << "Number of points must be divisible by number of ranks.\n";
    return 1;
  }
  const int iterations = std::stoi(argv[3]);

  // Read routing tables
  std::vector<std::vector<char>> routing_tables_ckr(
      kChannelsPerRank, std::vector<char>(kNumTags*2));
  std::vector<std::vector<char>> routing_tables_cks(
      kChannelsPerRank, std::vector<char>(mpi_size));
  for (int i = 0; i < kChannelsPerRank; ++i) {
    LoadRoutingTable<char>(mpi_rank, i, kNumTags*2, "smi-routes", "ckr",
                           &routing_tables_ckr[i][0]);
    LoadRoutingTable<char>(mpi_rank, i, mpi_size, "smi-routes", "cks",
                           &routing_tables_cks[i][0]);
  }

  AlignedVec_t input; 
  AlignedVec_t points(kDims * points_per_rank);
  AlignedVec_t centroids(kK * kDims);
  if (mpi_rank == 0) {
    MPIStatus(mpi_rank, "Initializing host memory...\n");
    // std::random_device rd;
    std::default_random_engine rng(5);
    input = AlignedVec_t(num_points * kDims);  // TODO: load some data set
    // Generate Gaussian means with a uniform distribution
    std::uniform_real_distribution<Data_t> dist_means(-5, 5);
    AlignedVec_t gaussian_means(kK * kDims);
    // Randomize centers for Gaussian distributions
    for (int k = 0; k < kK; ++k) {
      for (int d = 0; d < kDims; ++d) {
        gaussian_means[k * kDims + d] = dist_means(rng);
      }
    }
    // Print source means
    std::stringstream ss;
    ss << "Means used to generate data:\n";
    for (int k = 0; k < kK; ++k) {
      ss << "  {" << gaussian_means[k * kDims];
      for (int d = 1; d < kDims; ++d) {
        ss << ", " << gaussian_means[k * kDims + d];
      }
      ss << "}\n";
    }
    MPIStatus(mpi_rank, ss.str());
    // Generate data with a separate Gaussian at each mean
    std::normal_distribution<Data_t> normal_dist;
    /*for (int k = 0; k < kK; ++k) {
      const int n_per_centroid = num_points / kK;
      for (int i = k * n_per_centroid; i < (k + 1) * n_per_centroid; ++i) {
        for (int d = 0; d < kDims; ++d) {
          input[i * kDims + d] =
              normal_dist(rng) + gaussian_means[k * kDims + d];
        }
      }
    }*/
    for(int i=0;i<num_points;i++)
    {
        int k=i%kK;
        for (int d = 0; d < kDims; ++d) {
            input[i * kDims + d] =
                    normal_dist(rng) + gaussian_means[k * kDims + d];
        }
    }
    // For sampling indices for starting centroids
    std::uniform_int_distribution<size_t> index_dist(0, num_points);
    // Choose initial centroids
    for (int k = 0; k < kK; ++k) {
      auto i = index_dist(rng);
      std::copy(&input[i * kDims], &input[(i + 1) * kDims],
                &centroids[k * kDims]);
    }
    // Print starting centroids
    ss.str("");
    ss.clear();
    ss << "Means used to generate data:\n";
    ss << "Initial centroids:\n";
    for (int k = 0; k < kK; ++k) {
      ss << "  {" << centroids[k * kDims];
      for (int d = 1; d < kDims; ++d) {
        ss << ", " << centroids[k * kDims + d];
      }
      ss << "}\n";
    }
    MPIStatus(mpi_rank, ss.str());
  }
  // Distribute data
  MPIStatus(mpi_rank, "Communicating host memory...\n");
  MPI_Bcast(centroids.data(), kK * kDims, MPI_FLOAT, 0, MPI_COMM_WORLD);
  MPI_Scatter(input.data(), points_per_rank* kDims, MPI_FLOAT, points.data(),
              points_per_rank* kDims, MPI_FLOAT, 0, MPI_COMM_WORLD);

  try {

    MPIStatus(mpi_rank, "Creating OpenCL context...\n");
    hlslib::ocl::Context context(emulator ? 0 : (mpi_rank % kDevicesPerNode));

    MPIStatus(mpi_rank, "Allocating and copying device memory...\n");
    auto points_device = context.MakeBuffer<Data_t, hlslib::ocl::Access::read>(
        points.cbegin(), points.cend());
    auto centroids_device_read =
        context.MakeBuffer<Data_t, hlslib::ocl::Access::read>(centroids.cbegin(),
                                                              centroids.cend());
    auto centroids_device_write =
        context.MakeBuffer<Data_t, hlslib::ocl::Access::write>(centroids.cbegin(),
                                                               centroids.cend());
    std::vector<hlslib::ocl::Buffer<char, hlslib::ocl::Access::read>>
        routing_tables_cks_device(kChannelsPerRank);
    std::vector<hlslib::ocl::Buffer<char, hlslib::ocl::Access::read>>
        routing_tables_ckr_device(kChannelsPerRank);
    for (int i = 0; i < kChannelsPerRank; ++i) {
      routing_tables_cks_device[i] =
          context.MakeBuffer<char, hlslib::ocl::Access::read>(
              routing_tables_cks[i].cbegin(), routing_tables_cks[i].cend());
      routing_tables_ckr_device[i] =
          context.MakeBuffer<char, hlslib::ocl::Access::read>(
              routing_tables_ckr[i].cbegin(), routing_tables_ckr[i].cend());
    }

    MPIStatus(mpi_rank, "Creating program from binary...\n");
    auto program = context.MakeProgram(kernel_path);

    MPIStatus(mpi_rank, "Starting communication kernels...\n");
    std::vector<hlslib::ocl::Kernel> comm_kernels;

     for (int i = 0; i < kChannelsPerRank; ++i) {
      comm_kernels.emplace_back(program.MakeKernel(
          "smi_kernel_cks_" + std::to_string(i), routing_tables_cks_device[i],((char)mpi_size)));
      comm_kernels.emplace_back(program.MakeKernel("smi_kernel_ckr_" + std::to_string(i),
                                                   routing_tables_ckr_device[i],
                                                   (char)(mpi_rank)));
    }
    char mpi_size_comm = mpi_size;
    comm_kernels.emplace_back(
        program.MakeKernel("smi_kernel_reduce_0", mpi_size_comm));
    comm_kernels.emplace_back(
        program.MakeKernel("smi_kernel_bcast_1", mpi_size_comm));
    comm_kernels.emplace_back(
        program.MakeKernel("smi_kernel_reduce_2", mpi_size_comm));
    comm_kernels.emplace_back(
        program.MakeKernel("smi_kernel_bcast_3", mpi_size_comm));
    for (auto &k : comm_kernels) {
      // Will never terminate, so we don't care about the return value of fork
    //  k.ExecuteTaskFork(); //HLSLIB
        cl::CommandQueue queue=k.commandQueue();
        queue.enqueueTask(k.kernel());
        queue.flush();

    }

    // Wait for communication kernels to start
    MPI_Barrier(MPI_COMM_WORLD);

    MPIStatus(mpi_rank, "Creating compute kernels...\n");
    SMI_Comm comm{(char)mpi_rank,(char)mpi_size};

    std::vector<hlslib::ocl::Kernel> kernels;
    kernels.emplace_back(program.MakeKernel("SendCentroids",
                                            centroids_device_read, iterations,
                                            mpi_rank, mpi_size));
    kernels.emplace_back(program.MakeKernel("ComputeDistance", points_device,
                                            points_per_rank, iterations,
                                            mpi_rank, mpi_size));
    kernels.emplace_back(
        program.MakeKernel("ComputeMeans", centroids_device_write,
                           points_per_rank, iterations, comm));

    // Wait for compute kernels to be initialized
    MPI_Barrier(MPI_COMM_WORLD);

    // Execute kernel
    MPIStatus(mpi_rank, "Launching compute kernels...\n");
    std::vector<std::future<std::pair<double, double>>> futures;
    const auto start = std::chrono::high_resolution_clock::now();
    cl::Event events[3];

    //for (auto &k : kernels) {
    for(int i=0;i<3;i++){
      //futures.emplace_back(k.ExecuteTaskAsync()); //HLSLIB
        cl::CommandQueue queue=kernels[i].commandQueue();
        queue.enqueueTask(kernels[i].kernel(),nullptr, &events[i]);
        //queue.flush();
    }

    MPIStatus(mpi_rank, "Waiting for kernels to finish...\n");
    //hlslib
    /*for (auto &f : futures) {
      f.wait();
    }*/
    //for (auto &k : kernels) {
    for(int i=0;i<3;i++){
      //futures.emplace_back(k.ExecuteTaskAsync()); HLSLIB
        //cl::CommandQueue queue=k.commandQueue();
        //queue.finish();
        events[i].wait();
    }
    std::cout << mpi_rank <<" finished"<<std::endl;
    const auto end = std::chrono::high_resolution_clock::now();
    const double elapsed =
        1e-9 *
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    MPIStatus(mpi_rank, "Finished in ", elapsed, " seconds.\n");

    // Make sure all kernels finished
    MPI_Barrier(MPI_COMM_WORLD);

    // Copy back result
    MPIStatus(mpi_rank, "Copying back result...\n");
    centroids_device_write.CopyToHost(centroids.begin());

  } catch (std::exception const &err) {
    // Don't exit immediately, such that MPI will not exit other ranks that are
    // currently reconfiguring the FPGA.
    std::cerr << err.what() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 1;
  }

  if (mpi_rank == 0) {

    // Final centroids
    std::cout << "Final centroids:\n";
    for (int k = 0; k < kK; ++k) {
      std::cout << "  {" << centroids[k * kDims];
      for (int d = 1; d < kDims; ++d) {
        std::cout << ", " << centroids[k * kDims + d];
      }
      std::cout << "}\n";
    }

  }

  MPI_Finalize();

  return 0;
}

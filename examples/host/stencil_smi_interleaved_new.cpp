#include <mpi.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <numeric>
#include <sstream>
#include <vector>
//#include "common.h"
#include "hlslib/intel/OpenCL.h"
#include "stencil.h"
#include "../../include/utils/ocl_utils.hpp"
#include "../../include/utils/utils.hpp"
#include "../../include/utils/smi_utils.hpp"

// Convert from C to C++
using Data_t = DTYPE;
constexpr Data_t kBoundary = BOUNDARY_VALUE;
constexpr auto kMemoryBanks = B;
constexpr int kX = X;
constexpr int kY = Y;
constexpr int kW = W;
constexpr int kPX = PX;
constexpr int kPY = PY;
constexpr int kXLocal = kX / kPX;
constexpr int kYLocal = kY / kPY;
constexpr auto kDevicesPerNode = SMI_DEVICES_PER_NODE;
constexpr auto kUsage =
    "Usage: ./stencil_smi_interleaved <[emulator/hardware]> <num timesteps>\n";

using AlignedVec_t =
    std::vector<Data_t, hlslib::ocl::AlignedAllocator<Data_t, 64>>;

// Reference implementation for checking correctness
void Reference(AlignedVec_t &domain, const int timesteps) {
  AlignedVec_t buffer(domain);
  for (int t = 0; t < timesteps; ++t) {
    for (int i = 1; i < kX - 1; ++i) {
      for (int j = 1; j < kY - 1; ++j) {
        buffer[i * kY + j] =
            static_cast<Data_t>(0.25) *
            (domain[(i - 1) * kY + j] + domain[(i + 1) * kY + j] +
             domain[i * kY + j - 1] + domain[i * kY + j + 1]);
      }
    }
    domain.swap(buffer);
  }
}

std::vector<AlignedVec_t> SplitMemory(AlignedVec_t const &memory) {
  std::vector<AlignedVec_t> split(kPX * kPY,
                                  AlignedVec_t((kXLocal) * (kYLocal)));
  for (int px = 0; px < kPX; ++px) {
    for (int py = 0; py < kPY; ++py) {
      for (int x = 0; x < kXLocal; ++x) {
        for (int y = 0; y < kYLocal; ++y) {
          split[px * kPY + py][x * kYLocal + y] =
              memory[px * kXLocal * kY + x * kY + py * kYLocal + y];
        }
      }
    }
  }
  return split;
}

std::vector<AlignedVec_t> InterleaveMemory(AlignedVec_t const &memory) {
  std::vector<AlignedVec_t> split(
      kMemoryBanks, AlignedVec_t(kXLocal * kYLocal / kMemoryBanks));
  for (int x = 0; x < kXLocal; ++x) {
    for (int y = 0; y < kYLocal / (kMemoryBanks * kW); ++y) {
      for (int b = 0; b < kMemoryBanks; ++b) {
        for (int w = 0; w < kW; ++w) {
          split[b][x * kYLocal / kMemoryBanks + y * kW + w] =
              memory[x * kYLocal + y * kMemoryBanks * kW + b * kW + w];
        }
      }
    }
  }
  return split;
}

AlignedVec_t CombineMemory(std::vector<AlignedVec_t> const &split) {
  AlignedVec_t memory(kX * kY);
  for (int px = 0; px < kPX; ++px) {
    for (int py = 0; py < kPY; ++py) {
      for (int x = 0; x < kXLocal; ++x) {
        for (int y = 0; y < kYLocal; ++y) {
          memory[px * kXLocal * kY + x * kY + py * kYLocal + y] =
              split[px * kPY + py][x * kYLocal + y];
        }
      }
    }
  }
  return memory;
}

AlignedVec_t DeinterleaveMemory(std::vector<AlignedVec_t> const &split) {
  AlignedVec_t memory(kXLocal * kYLocal);
  for (int x = 0; x < kXLocal; ++x) {
    for (int y = 0; y < kYLocal / (kMemoryBanks * kW); ++y) {
      for (int b = 0; b < kMemoryBanks; ++b) {
        for (int w = 0; w < kW; ++w) {
          memory[x * kYLocal + y * kMemoryBanks * kW + b * kW + w] =
              split[b][x * kYLocal / kMemoryBanks + y * kW + w];
        }
      }
    }
  }
  return memory;
}

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
  std::cout << "Executing stencil "<<kX<<"x"<<kY<<", with a grid of "<<kPX<<"x"<<kPY<<" ranks"<<std::endl;
  MPI_Init(&argc, &argv);

  int mpi_size, mpi_rank;
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
   int i_px = mpi_rank / kPY;
   int i_py = mpi_rank % kPY;

  // Handle input arguments
  if (argc != 3) {
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
    kernel_path = ("stencil_smi_interleaved_emulator_" +
                   std::to_string(mpi_rank) + ".aocx");
  } else if (mode_str == "hardware") {
    kernel_path = "stencil_smi_interleaved_hardware.aocx";
    emulator = false;
  } else {
    std::cout << kUsage;
    return 2;
  }

   int timesteps = std::stoi(argv[2]);
  int fpga = mpi_rank % 2;

  // Read routing tables
  char routing_tables_ckr[4][8]; //only one tag
    char routing_tables_cks[4][mpi_size];
    for (int i = 0; i < kChannelsPerRank; ++i) {
        LoadRoutingTable<char>(mpi_rank, i, 8, "stencil_smi_interleaved_routing", "ckr", &routing_tables_ckr[i][0]);
        LoadRoutingTable<char>(mpi_rank, i, mpi_size, "stencil_smi_interleaved_routing", "cks", &routing_tables_cks[i][0]);
    }


  std::vector<AlignedVec_t> host_buffers;
  AlignedVec_t host_buffer, reference;
  if (mpi_rank == 0) {
    MPIStatus(mpi_rank, "Initializing host memory...\n");
    // Set center to 0
    reference = AlignedVec_t(kX * kY, 0);
    // Set boundaries to 1
    for (int i = 0; i < kY; ++i) {
      reference[i] = 1;
      reference[kY * (kX - 1) + i] = 1;
    }
    for (int i = 0; i < kX; ++i) {
      reference[i * kY] = 1;
      reference[i * kY + kY - 1] = 1;
    }
    host_buffers = SplitMemory(reference);
    host_buffer = std::move(host_buffers[0]);
    MPIStatus(mpi_rank, "Communicating host memory...\n");
    for (int i = 1; i < mpi_size; ++i) {
      MPI_Send(host_buffers[i].data(), host_buffers[i].size(), MPI_FLOAT, i, 0,
               MPI_COMM_WORLD);
    }
  } else {
    MPIStatus(mpi_rank, "Communicating host memory...\n");
    host_buffer = AlignedVec_t(kXLocal * kYLocal);
    MPI_Status status;
    MPI_Recv(host_buffer.data(), host_buffer.size(), MPI_FLOAT, 0, 0,
             MPI_COMM_WORLD, &status);
  }

  MPIStatus(mpi_rank, "Interleaving memory...\n");
  auto interleaved_host = InterleaveMemory(host_buffer);

  MPIStatus(mpi_rank, "Creating OpenCL context...\n");
  try {
    MPIStatus(mpi_rank, "Creating program from binary...\n");
     cl::Platform  platform;
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::vector<cl::Kernel> kernels;
    std::vector<cl::CommandQueue> queues;
    std::vector<std::string> kernel_names={"CK_S_0", "CK_S_1", "CK_S_2", "CK_S_3", "CK_R_0", "CK_R_1", "CK_R_2", "CK_R_3",
                                          "ConvertReceiveLeft", "ConvertReceiveRight", "ConvertReceiveTop", "ConvertReceiveBottom", "ConvertSendLeft", "ConvertSendRight", "ConvertSendTop", "ConvertSendBottom",
                                          "Read", "Stencil","Write"};
    IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,kernel_path,kernel_names, kernels,queues);
                                     
    MPI_Barrier(MPI_COMM_WORLD);

    MPIStatus(mpi_rank, "Allocating device memory...\n");
    const std::array<hlslib::ocl::MemoryBank, 4> banks = {
        hlslib::ocl::MemoryBank::bank0, hlslib::ocl::MemoryBank::bank1,
        hlslib::ocl::MemoryBank::bank2, hlslib::ocl::MemoryBank::bank3};
    std::vector<hlslib::ocl::Buffer<Data_t, hlslib::ocl::Access::readWrite>>
        device_buffers;
    
    cl::Buffer device_buffer1(context,CL_MEM_READ_WRITE|CL_CHANNEL_1_INTELFPGA,sizeof(float)*(2 * kXLocal * kYLocal / kMemoryBanks));
    cl::Buffer device_buffer2(context,CL_MEM_READ_WRITE|CL_CHANNEL_2_INTELFPGA,sizeof(float)*(2 * kXLocal * kYLocal / kMemoryBanks));
    cl::Buffer device_buffer3(context,CL_MEM_READ_WRITE|CL_CHANNEL_3_INTELFPGA,sizeof(float)*(2 * kXLocal * kYLocal / kMemoryBanks));
    cl::Buffer device_buffer4(context,CL_MEM_READ_WRITE|CL_CHANNEL_4_INTELFPGA,sizeof(float)*(2 * kXLocal * kYLocal / kMemoryBanks));



    queues[0].enqueueWriteBuffer(device_buffer1, CL_TRUE,0,sizeof(float)*(kXLocal * kYLocal / kMemoryBanks),interleaved_host[0].data());
    queues[0].enqueueWriteBuffer(device_buffer1, CL_TRUE,kXLocal * kYLocal / kMemoryBanks*sizeof(float),sizeof(float)*(kXLocal * kYLocal / kMemoryBanks),interleaved_host[0].data());
    queues[0].enqueueWriteBuffer(device_buffer2, CL_TRUE,0,sizeof(float)*(kXLocal * kYLocal / kMemoryBanks),interleaved_host[1].data());
    queues[0].enqueueWriteBuffer(device_buffer2, CL_TRUE,kXLocal * kYLocal / kMemoryBanks*sizeof(float),sizeof(float)*(kXLocal * kYLocal / kMemoryBanks),interleaved_host[1].data());
    queues[0].enqueueWriteBuffer(device_buffer3, CL_TRUE,0,sizeof(float)*(kXLocal * kYLocal / kMemoryBanks),interleaved_host[2].data());
    queues[0].enqueueWriteBuffer(device_buffer3, CL_TRUE,kXLocal * kYLocal / kMemoryBanks*sizeof(float),sizeof(float)*(kXLocal * kYLocal / kMemoryBanks),interleaved_host[2].data());
    queues[0].enqueueWriteBuffer(device_buffer4, CL_TRUE,0,sizeof(float)*(kXLocal * kYLocal / kMemoryBanks),interleaved_host[3].data());
    queues[0].enqueueWriteBuffer(device_buffer4, CL_TRUE,kXLocal * kYLocal / kMemoryBanks*sizeof(float),sizeof(float)*(kXLocal * kYLocal / kMemoryBanks),interleaved_host[3].data());
    

    cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,mpi_size);
    cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,mpi_size);
    cl::Buffer routing_table_ck_s_2(context,CL_MEM_READ_ONLY,mpi_size);
    cl::Buffer routing_table_ck_s_3(context,CL_MEM_READ_ONLY,mpi_size);
    cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,8);
    cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,8);
    cl::Buffer routing_table_ck_r_2(context,CL_MEM_READ_ONLY,8);
    cl::Buffer routing_table_ck_r_3(context,CL_MEM_READ_ONLY,8);


    // std::pair<kernel object, whether kernel is autorun>
    std::vector<hlslib::ocl::Kernel> comm_kernels;

    MPIStatus(mpi_rank, "Starting communication kernels...\n");

    
    queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,mpi_size,&routing_tables_cks[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,mpi_size,&routing_tables_cks[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE,0,mpi_size,&routing_tables_cks[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE,0,mpi_size,&routing_tables_cks[3][0]);

    queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,8,&routing_tables_ckr[0][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,8,&routing_tables_ckr[1][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE,0,8,&routing_tables_ckr[2][0]);
    queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE,0,8,&routing_tables_ckr[3][0]);

    kernels[0].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
    kernels[0].setArg(1,sizeof(int),&mpi_size);
    kernels[1].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
    kernels[1].setArg(1,sizeof(int),&mpi_size);
    kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_2);
    kernels[2].setArg(1,sizeof(int),&mpi_size);
    kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_s_3);
    kernels[3].setArg(1,sizeof(int),&mpi_size);

    //args for the CK_Rs
    char my_rank=mpi_rank;
    kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
    kernels[4].setArg(1,sizeof(char),&my_rank);
    kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
    kernels[5].setArg(1,sizeof(char),&my_rank);
    kernels[6].setArg(0,sizeof(cl_mem),&routing_table_ck_r_2);
    kernels[6].setArg(1,sizeof(char),&my_rank);
    kernels[7].setArg(0,sizeof(cl_mem),&routing_table_ck_r_3);
    kernels[7].setArg(1,sizeof(char),&my_rank);

    //start the CKs
    for(int i=0;i<8;i++) //start also the bcast kernel
    {
        queues[i].enqueueTask(kernels[i]);
    }

    // Wait for communication kernels to start
    MPI_Barrier(MPI_COMM_WORLD);
    
    MPIStatus(mpi_rank, "Starting memory conversion kernels...\n");

    for(int i=8;i<16;i++)
    {
      kernels[i].setArg(0,sizeof(int),&i_px);
      kernels[i].setArg(1,sizeof(int),&i_py);
      queues[i].enqueueTask(kernels[i]);
    }

    // Wait for conversion kernels to start
    MPI_Barrier(MPI_COMM_WORLD);

    MPIStatus(mpi_rank, "Creating compute kernels...\n");

    //Read
    kernels[16].setArg(0,sizeof(cl_mem),&device_buffer1);
    kernels[16].setArg(1,sizeof(cl_mem),&device_buffer2);
    kernels[16].setArg(2,sizeof(cl_mem),&device_buffer3);
    kernels[16].setArg(3,sizeof(cl_mem),&device_buffer4);
    kernels[16].setArg(4,sizeof(int),&i_px);
    kernels[16].setArg(5,sizeof(int),&i_py);
    kernels[16].setArg(6,sizeof(int),&timesteps);

    //Stencil
    kernels[17].setArg(0,sizeof(int),&i_px);
    kernels[17].setArg(1,sizeof(int),&i_py);
    kernels[17].setArg(2,sizeof(int),&timesteps);

    //Write
    kernels[18].setArg(0,sizeof(cl_mem),&device_buffer1);
    kernels[18].setArg(1,sizeof(cl_mem),&device_buffer2);
    kernels[18].setArg(2,sizeof(cl_mem),&device_buffer3);
    kernels[18].setArg(3,sizeof(cl_mem),&device_buffer4);
    kernels[18].setArg(4,sizeof(int),&i_px);
    kernels[18].setArg(5,sizeof(int),&i_py);
    kernels[18].setArg(6,sizeof(int),&timesteps);

    // Wait for all ranks to be ready for launch
    MPI_Barrier(MPI_COMM_WORLD);

    MPIStatus(mpi_rank, "Launching kernels...\n");


    const auto start = std::chrono::high_resolution_clock::now();
    for(int i=16;i<19;i++) 
        queues[i].enqueueTask(kernels[i]);


    MPIStatus(mpi_rank, "Waiting for kernels to finish...\n");
    for(int i=16;i<19;i++)
    {

        queues[i].finish();
    }


    const auto end = std::chrono::high_resolution_clock::now();
    const double elapsed =
        1e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                   .count();
    MPIStatus(mpi_rank, "Finished in ", elapsed, " seconds.\n");

    // Make sure all kernels finished
    MPI_Barrier(MPI_COMM_WORLD);
    if (mpi_rank == 0) {
      const auto end_global = std::chrono::high_resolution_clock::now();
      const double elapsed =
          1e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(
                     end_global - start)
                     .count();
      MPIStatus(mpi_rank, "All ranks done in ", elapsed, " seconds.\n");
    }

    // Copy back result
    MPIStatus(mpi_rank, "Copying back result...\n");
    int offset = (timesteps % 2 == 0) ? 0 : kYLocal * kXLocal / kMemoryBanks;

    queues[18].enqueueReadBuffer(device_buffer1, CL_TRUE,offset*sizeof(float),sizeof(float)*(kYLocal * kXLocal / kMemoryBanks),interleaved_host[0].data());
    queues[18].enqueueReadBuffer(device_buffer2, CL_TRUE,offset*sizeof(float),sizeof(float)*(kYLocal * kXLocal / kMemoryBanks),interleaved_host[1].data());
    queues[18].enqueueReadBuffer(device_buffer3, CL_TRUE,offset*sizeof(float),sizeof(float)*(kYLocal * kXLocal / kMemoryBanks),interleaved_host[2].data());
    queues[18].enqueueReadBuffer(device_buffer4, CL_TRUE,offset*sizeof(float),sizeof(float)*(kYLocal * kXLocal / kMemoryBanks),interleaved_host[3].data());

  } catch (std::exception const &err) {
    // Don't exit immediately, such that MPI will not exit other ranks that are
    // currently reconfiguring the FPGA.
    std::cout << err.what() << std::endl; 
    std::this_thread::sleep_for(std::chrono::seconds(30));
    return 1;
  }

  MPIStatus(mpi_rank, "De-interleaving memory...\n");
  host_buffer = DeinterleaveMemory(interleaved_host);

  // Communicate
  MPIStatus(mpi_rank, "Communicating result...\n");
  if (mpi_rank == 0) {
    host_buffers[0] = std::move(host_buffer);
    for (int i = 1; i < mpi_size; ++i) {
      MPI_Status status;
      MPI_Recv(host_buffers[i].data(), host_buffers[i].size(), MPI_FLOAT, i, 0,
               MPI_COMM_WORLD, &status);
    }
  } else {
    MPI_Send(host_buffer.data(), host_buffer.size(), MPI_FLOAT, 0, 0,
             MPI_COMM_WORLD);
  }

  // Run reference implementation
  if (mpi_rank == 0) {
    MPIStatus(mpi_rank, "Running reference implementation...\n");
    Reference(reference, timesteps);

    const auto result = CombineMemory(host_buffers);

    // std::cout << "\n";
    // for (int i = 0; i < kX; ++i) {
    //   for (int j = 0; j < kY; ++j) {
    //     std::cout << result[i * kY + j] << " ";
    //   }
    //   std::cout << "\n";
    // }
    // std::cout << "\n";

    // Compare result
    const Data_t average =
        std::accumulate(reference.begin(), reference.end(), 0.0) /
        reference.size();
    for (int i = 0; i < kX; ++i) {
      for (int j = 0; j < kY; ++j) {
        const auto res = result[i * kY + j];
        const auto ref = reference[i * kY + j];
        if (std::abs(ref - res) >= 1e-4 * average) {
          std::cerr << "Mismatch found at (" << i << ", " << j << "): " << res
                    << " (should be " << ref << ").\n";
          return 3;
        }
      }
    }

    std::cout << "Successfully verified result.\n";
  }

  MPI_Finalize();

  return 0;
}

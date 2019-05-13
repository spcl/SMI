#include <mpi.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <numeric>
#include <sstream>
#include <vector>
//#include "common.h"
#include "hlslib/intel/OpenCL.h"
#include "hotspot.h"
#include "ocl_utils.hpp"
#include "utils.hpp"
#include "smi_utils.hpp"
#include <unistd.h>
const int HOST_NAME_MAX=256;
// Convert from C to C++
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
    "Usage: ./hotspot_smi <[emulator/hardware]> <num timesteps> <temperature "
    "in> <power in> <temperature out>\n";

// Constants
constexpr auto kChipHeight = 1.6;
constexpr auto kChipWidth = 1.6;
constexpr auto kAmbientTemperature = AMB_TEMP;
constexpr auto kTChip = 0.0005;
constexpr auto kKSI = 100;
constexpr auto kFactorChip = 0.5;
constexpr auto kPrecision = 0.001;
constexpr auto kSpecHeatSI = 1.75e6;
constexpr auto kMaxPD = 3.0e6;

using AlignedVec_t =
    std::vector<float, hlslib::ocl::AlignedAllocator<float, 64>>;

// Reference implementation for checking correctness
void Reference(AlignedVec_t &temperature, const AlignedVec_t &power,
               const float rx1, const float ry1, const float rz1,
               const float sdc, const int timesteps) {
  AlignedVec_t buffer(temperature);
  for (int t = 0; t < timesteps; ++t) {
    for (int i = 0; i < kX; ++i) {
      for (int j = 0; j < kY; ++j) {
        const float center = temperature[i * kY + j];
        const float north = (i > 0) ? temperature[(i - 1) * kY + j] : center;
        const float south =
            (i < kX - 1) ? temperature[(i + 1) * kY + j] : center;
        const float west = (j > 0) ? temperature[i * kY + j - 1] : center;
        const float east = (j < kY - 1) ? temperature[i * kY + j + 1] : center;
        const float v = power[i * kY + j] +
                        (north + south - 2.0f * center) * ry1 +
                        (east + west - 2.0f * center) * rx1 +
                        (kAmbientTemperature - center) * rz1;
        const float delta = sdc * v;
        buffer[i * kY + j] = center + delta;
      }
    }
    temperature.swap(buffer);
  }
}

AlignedVec_t ReadFile(const std::string path) {
  std::ifstream fstream(path);
  AlignedVec_t result(kX * kY);
  std::string line;
  for (int i = 0; i < kX * kY; ++i) {
    std::getline(fstream, line);
    result[i] = std::stof(line);
  }
  return result;
}

void WriteFile(const std::string path, AlignedVec_t const &data) {
  std::ofstream fstream(path);
  for (int i = 0; i < kX * kY; ++i) {
    fstream << i << "\t" << data[i] << "\n";
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
  MPI_Init(&argc, &argv);

  int mpi_size, mpi_rank;
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
   int i_px = mpi_rank / kPY;
   int i_py = mpi_rank % kPY;

  // Handle input arguments
  if (argc != 6) {
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
        ("hotspot_smi_emulator_" + std::to_string(mpi_rank) + ".aocx");
  } else if (mode_str == "hardware") {
    kernel_path = "hotspot_smi_hardware.aocx";
    emulator = false;
  } else {
    std::cout << kUsage;
    return 2;
  }

 int timesteps = std::stoi(argv[2]);

  const std::string temperature_file_in(argv[3]);
  const std::string power_file_in(argv[4]);
  const std::string temperature_file_out(argv[5]);

  // Compute hotspot values
   float grid_height = kChipHeight / kX;
   float grid_width = kChipWidth / kY;
   float rx1 = 1 / (grid_width / (2.0 * kKSI * kTChip * grid_height));
   float ry1 = 1 / (grid_height / (2.0 * kKSI * kTChip * grid_width));
   float rz1 = 1 / (kTChip / (kKSI * grid_height * grid_width));
   float cap =
      kFactorChip * kSpecHeatSI * kTChip * grid_width * grid_height;
   float max_slope = kMaxPD / (kFactorChip * kTChip * kSpecHeatSI);
   float step = kPrecision / max_slope;
   float step_div_cap = step / cap;

  // Read routing tables
  std::vector<std::vector<char>> routing_tables_ckr(kChannelsPerRank,
                                                    std::vector<char>(4));
  std::vector<std::vector<char>> routing_tables_cks(
      kChannelsPerRank, std::vector<char>(mpi_size));
  for (int i = 0; i < kChannelsPerRank; ++i) {
    LoadRoutingTable<char>(mpi_rank, i, 4, "hotspot_smi_routing", "ckr",
                           &routing_tables_ckr[i][0]);
    LoadRoutingTable<char>(mpi_rank, i, mpi_size, "hotspot_smi_routing", "cks",
                           &routing_tables_cks[i][0]);
  }

 /* usleep(100000*mpi_rank);
  std::cout << "----------------" <<std::endl;
  std::cout << "Rank: "<< mpi_rank<<std::endl;
  std::cout << "CK_R_TABLE: " <<std::endl;
  for(int i=0;i<kChannelsPerRank;i++)
      for(int j=0;j<4;j++)
          std::cout << i<< "," << j<<": "<< (int)routing_tables_ckr[i][j]<<std::endl;
  std::cout << "CK_S_TABLE: " <<std::endl;
  for(int i=0;i<kChannelsPerRank;i++)
      for(int j=0;j<mpi_size;j++)
          std::cout << i<< "," << j<<": "<< (int)routing_tables_cks[i][j]<<std::endl;
    sleep(1);
*/
  std::vector<AlignedVec_t> temperature_host_split, power_host_split;
  AlignedVec_t temperature_host, power_host, temperature_reference,
      power_reference;
  if (mpi_rank == 0) {
    MPIStatus(mpi_rank, "Initializing host memory...\n");
    // Set center to 0
    temperature_reference = ReadFile(temperature_file_in);
    power_reference = ReadFile(power_file_in);
    // temperature_reference = AlignedVec_t(kX * kY, 0);
    // power_reference = AlignedVec_t(kX * kY, 0);
    temperature_host_split = SplitMemory(temperature_reference);
    temperature_host = std::move(temperature_host_split[0]);
    power_host_split = SplitMemory(power_reference);
    power_host = std::move(power_host_split[0]);
    MPIStatus(mpi_rank, "Communicating host memory...\n");
    for (int i = 1; i < mpi_size; ++i) {
      MPI_Send(temperature_host_split[i].data(),
               temperature_host_split[i].size(), MPI_FLOAT, i, 0,
               MPI_COMM_WORLD);
      MPI_Send(power_host_split[i].data(),
               power_host_split[i].size(), MPI_FLOAT, i, 0,
               MPI_COMM_WORLD);
    }
  } else {
    MPIStatus(mpi_rank, "Communicating host memory...\n");
    temperature_host = AlignedVec_t(kXLocal * kYLocal);
    power_host = AlignedVec_t(kXLocal * kYLocal);
    MPI_Status status;
    MPI_Recv(temperature_host.data(), temperature_host.size(), MPI_FLOAT, 0, 0,
             MPI_COMM_WORLD, &status);
    MPI_Recv(power_host.data(), power_host.size(), MPI_FLOAT, 0, 0,
             MPI_COMM_WORLD, &status);
  }

  MPIStatus(mpi_rank, "Interleaving memory...\n");
  auto temperature_interleaved_host = InterleaveMemory(temperature_host);
  const auto power_interleaved_host = InterleaveMemory(power_host);
  int fpga = mpi_rank % 2; // in this case is ok, pay attention
  //fpga=0; //executed on 15 and 16
  std::cout << "Rank: " << mpi_rank << " out of " << mpi_size << " ranks" << std::endl;
  std::cout << "Program: " << kernel_path << " executed on fpga: "<<fpga<<std::endl;
  char hostname[HOST_NAME_MAX];
  gethostname(hostname, HOST_NAME_MAX);
  printf("Rank %d executing on host: %s\n",mpi_rank,hostname);
  cl::Platform  platform;
  cl::Device device;
  cl::Context context;
  cl::Program program;
  std::vector<cl::Kernel> kernels;
  std::vector<cl::CommandQueue> queues;
  std::vector<std::string> kernel_names={"CK_S_0","CK_S_1","CK_S_2","CK_R_0","CK_R_1","CK_R_2","ConvertReceiveLeft",\
                                         "ConvertReceiveRight", "ConvertReceiveTop","ConvertReceiveBottom", "ConvertSendLeft","ConvertSendRight",\
                                         "ConvertSendTop","ConvertSendBottom","Read","ReadPower","Stencil","Write"};



  //this is for the case with classi channels
  IntelFPGAOCLUtils::initEnvironment(platform,device,fpga,context,program,kernel_path,kernel_names, kernels,queues);

  MPI_Barrier(MPI_COMM_WORLD);

  MPIStatus(mpi_rank, "Allocating device memory...\n");
  char tags=4;
  cl::Buffer routing_table_ck_s_0(context,CL_MEM_READ_ONLY,mpi_size);
  cl::Buffer routing_table_ck_s_1(context,CL_MEM_READ_ONLY,mpi_size);
  cl::Buffer routing_table_ck_s_2(context,CL_MEM_READ_ONLY,mpi_size);
  cl::Buffer routing_table_ck_s_3(context,CL_MEM_READ_ONLY,mpi_size);
  cl::Buffer routing_table_ck_r_0(context,CL_MEM_READ_ONLY,tags);
  cl::Buffer routing_table_ck_r_1(context,CL_MEM_READ_ONLY,tags);
  cl::Buffer routing_table_ck_r_2(context,CL_MEM_READ_ONLY,tags);
  cl::Buffer routing_table_ck_r_3(context,CL_MEM_READ_ONLY,tags);

  cl::Buffer temperature_interleaved_device[4], power_interleaved_device[4];
  temperature_interleaved_device[0]=cl::Buffer(context,CL_MEM_READ_WRITE,sizeof(float)* 2 * kXLocal * kYLocal / kMemoryBanks);
  temperature_interleaved_device[1]=cl::Buffer(context,CL_MEM_READ_WRITE,sizeof(float)* 2 * kXLocal * kYLocal / kMemoryBanks);
  temperature_interleaved_device[2]=cl::Buffer(context,CL_MEM_READ_WRITE,sizeof(float)* 2 * kXLocal * kYLocal / kMemoryBanks);
  temperature_interleaved_device[3]=cl::Buffer(context,CL_MEM_READ_WRITE,sizeof(float)* 2 * kXLocal * kYLocal / kMemoryBanks);
  power_interleaved_device[0]=cl::Buffer(context,CL_MEM_READ_ONLY, sizeof(float)* power_interleaved_host[0].size());
  power_interleaved_device[1]=cl::Buffer(context,CL_MEM_READ_ONLY, sizeof(float)* power_interleaved_host[1].size());
  power_interleaved_device[2]=cl::Buffer(context,CL_MEM_READ_ONLY, sizeof(float)* power_interleaved_host[2].size());
  power_interleaved_device[3]=cl::Buffer(context,CL_MEM_READ_ONLY, sizeof(float)* power_interleaved_host[3].size());

  for (int b = 0; b < kMemoryBanks; ++b) {
     queues[0].enqueueWriteBuffer(temperature_interleaved_device[b], CL_TRUE,0,sizeof(float)* kXLocal * kYLocal / kMemoryBanks,temperature_interleaved_host[b].data());
     queues[0].enqueueWriteBuffer(temperature_interleaved_device[b], CL_TRUE,sizeof(float)* kXLocal * kYLocal / kMemoryBanks,sizeof(float)* kXLocal * kYLocal / kMemoryBanks,temperature_interleaved_host[b].data());
     queues[0].enqueueWriteBuffer(power_interleaved_device[b],CL_TRUE,0 ,sizeof(float)* power_interleaved_host[b].size(), power_interleaved_host[b].data());
   }

  queues[0].enqueueWriteBuffer(routing_table_ck_s_0, CL_TRUE,0,mpi_size,&routing_tables_cks[0][0]);
  queues[0].enqueueWriteBuffer(routing_table_ck_s_1, CL_TRUE,0,mpi_size,&routing_tables_cks[1][0]);
  queues[0].enqueueWriteBuffer(routing_table_ck_s_2, CL_TRUE,0,mpi_size,&routing_tables_cks[2][0]);
  queues[0].enqueueWriteBuffer(routing_table_ck_s_3, CL_TRUE,0,mpi_size,&routing_tables_cks[3][0]);

  queues[0].enqueueWriteBuffer(routing_table_ck_r_0, CL_TRUE,0,4,&routing_tables_ckr[0]);
  queues[0].enqueueWriteBuffer(routing_table_ck_r_1, CL_TRUE,0,4,&routing_tables_ckr[1]);
  queues[0].enqueueWriteBuffer(routing_table_ck_r_2, CL_TRUE,0,4,&routing_tables_ckr[2]);
  queues[0].enqueueWriteBuffer(routing_table_ck_r_3, CL_TRUE,0,4,&routing_tables_ckr[3]);
 

  //LAUNCH CK
   //args for the CK_Ss
  kernels[0].setArg(0,sizeof(cl_mem),&routing_table_ck_s_0);
  kernels[1].setArg(0,sizeof(cl_mem),&routing_table_ck_s_1);
  kernels[2].setArg(0,sizeof(cl_mem),&routing_table_ck_s_2);
  

  //args for the CK_Rs
  kernels[3].setArg(0,sizeof(cl_mem),&routing_table_ck_r_0);
  kernels[3].setArg(1,sizeof(char),&mpi_rank);
  kernels[4].setArg(0,sizeof(cl_mem),&routing_table_ck_r_1);
  kernels[4].setArg(1,sizeof(char),&mpi_rank);
  kernels[5].setArg(0,sizeof(cl_mem),&routing_table_ck_r_2);
  kernels[5].setArg(1,sizeof(char),&mpi_rank);

  for(int i=0;i<6;i++)
     queues[i].enqueueTask(kernels[i]);

  // Wait for communication kernels to start
  MPI_Barrier(MPI_COMM_WORLD);

  MPIStatus(mpi_rank, "Starting memory conversion kernels...\n");

  //convert receive
  for(int i=6;i<14;i++)
  {
    kernels[i].setArg(0,sizeof(int),&i_px);
    kernels[i].setArg(1,sizeof(int),&i_py);
    kernels[i].setArg(2,sizeof(char),&mpi_rank);
  }
  for(int i=6;i<14;i++)
    queues[i].enqueueTask(kernels[i]);
  // Wait for conversion kernels to start
  MPI_Barrier(MPI_COMM_WORLD);

  MPIStatus(mpi_rank, "Creating compute kernels...\n");

  //compute kernels
  //Read
  for(int i=0;i<4;i++)
    kernels[14].setArg(i,sizeof(cl_mem),&(temperature_interleaved_device[i]));
  kernels[14].setArg(4,sizeof(int),&i_px);
  kernels[14].setArg(5,sizeof(int),&i_py);
  kernels[14].setArg(6,sizeof(int),&timesteps);
  kernels[14].setArg(7,sizeof(char),&mpi_rank);
  //Read power
  for(int i=0;i<4;i++)
    kernels[15].setArg(i,sizeof(cl_mem),&power_interleaved_device[i]);
  kernels[15].setArg(4,sizeof(int),&timesteps);
  kernels[15].setArg(5,sizeof(char),&mpi_rank);
  //stencil
  kernels[16].setArg(0,sizeof(float),&rx1);
  kernels[16].setArg(1,sizeof(float),&ry1);
  kernels[16].setArg(2,sizeof(float),&rz1);
  kernels[16].setArg(3,sizeof(float),&step_div_cap);
  kernels[16].setArg(4,sizeof(int),&i_px);
  kernels[16].setArg(5,sizeof(int),&i_py);
  kernels[16].setArg(6,sizeof(int),&timesteps);
  kernels[16].setArg(7,sizeof(char),&mpi_rank);

  //write
  for(int i=0;i<4;i++)
    kernels[17].setArg(i,sizeof(cl_mem),&temperature_interleaved_device[i]);
  kernels[17].setArg(4,sizeof(int),&i_px);
  kernels[17].setArg(5,sizeof(int),&i_py);
  kernels[17].setArg(6,sizeof(int),&timesteps);
  kernels[17].setArg(7,sizeof(char),&mpi_rank);


  // Wait for all ranks to be ready for launch
  MPI_Barrier(MPI_COMM_WORLD);

  MPIStatus(mpi_rank, "Launching kernels...\n");

  std::vector<std::future<std::pair<double, double>>> futures;
  const auto start = std::chrono::high_resolution_clock::now();
  for(int i=14;i<18;i++)
    queues[i].enqueueTask(kernels[i]);

  MPIStatus(mpi_rank, "Waiting for kernels to finish...\n");
  for(int i=14;i<18;i++)
    queues[i].finish();

  const auto end = std::chrono::high_resolution_clock::now();
  const double elapsed =
      1e-9 *
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  MPIStatus(mpi_rank, "Finished in ", elapsed, " seconds.\n");

  // Make sure all kernels finished
  MPI_Barrier(MPI_COMM_WORLD);
  if (mpi_rank == 0) {
    const auto end_global = std::chrono::high_resolution_clock::now();
    const double elapsed =
        1e-9 *
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_global - start)
            .count();
    MPIStatus(mpi_rank, "All ranks done in ", elapsed, " seconds.\n");
  }

  // Copy back result
  MPIStatus(mpi_rank, "Copying back result...\n");
  int offset = (timesteps % 2 == 0) ? 0 : kYLocal * kXLocal / kMemoryBanks;
  for (int b = 0; b < kMemoryBanks; ++b) {
    queues[0].enqueueReadBuffer(temperature_interleaved_device[b],CL_TRUE,offset,sizeof(float)*kYLocal * kXLocal / kMemoryBanks,temperature_interleaved_host[b].data());

  }

  MPIStatus(mpi_rank, "De-interleaving memory...\n");
  temperature_host = DeinterleaveMemory(temperature_interleaved_host);

  // Communicate
  MPIStatus(mpi_rank, "Communicating result...\n");
  if (mpi_rank == 0) {
    temperature_host_split[0] = std::move(temperature_host);
    for (int i = 1; i < mpi_size; ++i) {
      MPI_Status status;
      MPI_Recv(temperature_host_split[i].data(),
               temperature_host_split[i].size(), MPI_FLOAT, i, 0,
               MPI_COMM_WORLD, &status);
    }
  } else {
    MPI_Send(temperature_host.data(), temperature_host.size(), MPI_FLOAT, 0, 0,
             MPI_COMM_WORLD);
  }

  // Run reference implementation
  if (mpi_rank == 0) {
    MPIStatus(mpi_rank, "Running reference implementation...\n");
    Reference(temperature_reference, power_reference, rx1, ry1, rz1,
              step_div_cap, timesteps);

    const auto result = CombineMemory(temperature_host_split);

    // std::cout << "\n";
    // for (int i = 0; i < kX; ++i) {
    //   for (int j = 0; j < kY; ++j) {
    //     std::cout << result[i * kY + j] << " ";
    //   }
    //   std::cout << "\n";
    // }
    // std::cout << "\n";

    WriteFile(temperature_file_out, result);
    std::cout << "Wrote result to " << temperature_file_out << ".\n";

    // Compare result
    const float average = std::accumulate(temperature_reference.begin(),
                                          temperature_reference.end(), 0.0) /
                          temperature_reference.size();
    for (int i = 0; i < kX; ++i) {
      for (int j = 0; j < kY; ++j) {
        const auto res = result[i * kY + j];
        const auto ref = temperature_reference[i * kY + j];
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

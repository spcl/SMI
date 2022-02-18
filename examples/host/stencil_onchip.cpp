#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>
#include "hlslib/intel/OpenCL.h"
#include "stencil.h"
#include <utils/ocl_utils.hpp>

// Convert from C to C++
using Data_t = DTYPE;
constexpr Data_t kBoundary = BOUNDARY_VALUE;
constexpr int kX = X;
constexpr int kY = Y;
constexpr int kPX = PX;
constexpr int kPY = PY;
constexpr int kXLocal = kX / kPX;
constexpr int kYLocal = kY / kPY;
constexpr auto kUsage =
    "Usage: ./stencil_spatial_tiling <[emulator/hardware]> <num timesteps>\n";

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

int main(int argc, char **argv) {
  // Handle input arguments
  if (argc != 3) {
    std::cout << kUsage;
    return 1;
  }
  bool emulator = false;
  std::string mode_str(argv[1]);
  std::string kernel_path;
  if (mode_str == "emulator") {
    emulator = true;
    kernel_path = "emulator/stencil_onchip.aocx";
  } else if (mode_str == "hardware") {
    // TODO: find the right path
    kernel_path = "stencil_onchip.aocx";
    emulator = false;
  } else {
    std::cout << kUsage;
    return 2;
  }

  const int timesteps = std::stoi(argv[2]);

  std::cout << "Initializing host memory...\n" << std::flush;
  // Set center to 0
  AlignedVec_t reference(kX * kY, 0);
  // Set boundaries to 1
  for (int i = 0; i < kY; ++i) {
    reference[i] = 1;
    reference[kY * (kX - 1) + i] = 1;
  }
  for (int i = 0; i < kX; ++i) {
    reference[i * kY] = 1;
    reference[i * kY + kY - 1] = 1;
  }
  auto host_buffers = SplitMemory(reference);

  // Create OpenCL kernels
  std::cout << "Creating OpenCL context...\n" << std::flush;
  hlslib::ocl::Context *context;
  if (emulator) {
      context = new hlslib::ocl::Context(VENDOR_STRING_EMULATION, 0);
  } else {
      context = new hlslib::ocl::Context(VENDOR_STRING, 0);
  }
  std::cout << "Allocating device memory...\n" << std::flush;
  std::cout << "Creating program from binary...\n" << std::flush;
  auto program = context->MakeProgram(kernel_path);
  std::cout << "Creating kernels...\n" << std::flush;
  std::vector<hlslib::ocl::Kernel> kernels;
  std::vector<hlslib::ocl::Buffer<Data_t, hlslib::ocl::Access::readWrite>>
      device_buffers;
  std::array<hlslib::ocl::MemoryBank, 4> banks = {
      hlslib::ocl::MemoryBank::bank0, hlslib::ocl::MemoryBank::bank1,
      hlslib::ocl::MemoryBank::bank2, hlslib::ocl::MemoryBank::bank3};
  for (int i = 0; i < kPX; ++i) {
    for (int j = 0; j < kPY; ++j) {
      auto device_buffer =
          context->MakeBuffer<Data_t, hlslib::ocl::Access::readWrite>(
              banks[(i * kPY + j) % banks.size()], 2 * kXLocal * kYLocal);
      const std::string suffix("_" + std::to_string(i) + "_" +
                               std::to_string(j));
      kernels.emplace_back(
          program.MakeKernel("Read" + suffix, device_buffer, timesteps));
      kernels.emplace_back(program.MakeKernel("Stencil" + suffix, timesteps));
      kernels.emplace_back(
          program.MakeKernel("Write" + suffix, device_buffer, timesteps));
      device_buffers.emplace_back(std::move(device_buffer));
    }
  }
  std::cout << "Copying data to device...\n" << std::flush;
  for (int i = 0; i < kPX; ++i) {
    for (int j = 0; j < kPY; ++j) {
      // Copy to both sections of device memory, so that the boundary conditions
      // are reflected in both
      auto &device_buffer = device_buffers[i * kPY + j];
      auto &host_buffer = host_buffers[i * kPY + j];
      device_buffer.CopyFromHost(0, kXLocal * kYLocal, host_buffer.cbegin());
      device_buffer.CopyFromHost(kXLocal * kYLocal, kXLocal * kYLocal,
                                 host_buffer.cbegin());
    }
  }

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
  int offset = (timesteps % 2 == 0) ? 0 : kYLocal * kXLocal;
  for (int i = 0; i < kPX; ++i) {
    for (int j = 0; j < kPY; ++j) {
      auto &device_buffer = device_buffers[i * kPY + j];
      auto &host_buffer = host_buffers[i * kPY + j];
      device_buffer.CopyToHost(offset, kYLocal * kXLocal, host_buffer.begin());
    }
  }

  // Run reference implementation
  std::cout << "Running reference implementation...\n" << std::flush;
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

  return 0;
}

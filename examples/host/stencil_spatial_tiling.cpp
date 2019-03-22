#include <cmath>
#include <iostream>
#include "hlslib/intel/OpenCL.h"
#include "stencil.h"

// Convert from C to C++
using Data_t = DTYPE;
constexpr Data_t kBoundary = BOUNDARY_VALUE;
constexpr int kX = X;
constexpr int kY = Y;
constexpr int kT = T;
constexpr int kPX = PX;
constexpr int kPY = PY;
constexpr int kXLocal = kX / kPX;
constexpr int kYLocal = kY / kPY;
constexpr auto kUsage =
    "Usage: ./stencil_spatial_tiling <[emulator/hardware]>\n";

// Reference implementation for checking correctness
void Reference(std::vector<Data_t> &domain) {
  std::vector<Data_t> buffer(domain);
  for (int t = 0; t < T; ++t) {
    for (int i = 0; i < kX; ++i) {
      for (int j = 0; j < kY; ++j) {
        const auto left = (i > 0) ? domain[(i - 1) * kY + j] : kBoundary;
        const auto right = (i < kX - 1) ? domain[(i + 1) * kY + j] : kBoundary;
        const auto top = (j > 0) ? domain[i * kY + j - 1] : kBoundary;
        const auto bottom = (j < kY - 1) ? domain[i * kY + j + 1] : kBoundary;
        buffer[i * kY + j] =
            static_cast<Data_t>(0.25) * (left + right + top + bottom);
      }
    }
    domain.swap(buffer);
  }
}

std::vector<std::vector<Data_t>> SplitMemory(
    std::vector<Data_t> const &memory) {
  std::vector<std::vector<Data_t>> split(
      kPX * kPY, std::vector<Data_t>((kXLocal) * (kYLocal)));
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

std::vector<Data_t> CombineMemory(
    std::vector<std::vector<Data_t>> const &split) {
  std::vector<Data_t> memory(kX * kY);
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
  if (argc != 2) {
    std::cout << kUsage;
    return 1;
  }
  bool emulator = false;
  std::string mode_str(argv[1]);
  std::string kernel_path;
  if (mode_str == "emulator") {
    emulator = true;
    kernel_path = "stencil_spatial_tiling_emulator.aocx";
  } else if (mode_str == "hardware") {
    kernel_path = "stencil_spatial_tiling_hardware.aocx";
    emulator = false;
  } else {
    std::cout << kUsage;
    return 2;
  }

  std::cout << "Initializing host memory...\n" << std::flush;
  // Set center to 0
  std::vector<Data_t> reference(kX * kY, 0);
  auto host_buffers = SplitMemory(reference);

  // Create OpenCL kernels
  std::cout << "Creating OpenCL context...\n" << std::flush;
  hlslib::ocl::Context context;
  std::cout << "Allocating device memory...\n" << std::flush;
  std::cout << "Creating program from binary...\n" << std::flush;
  auto program = context.MakeProgram(kernel_path);
  std::cout << "Creating kernels...\n" << std::flush;
  std::vector<hlslib::ocl::Kernel> kernels;
  std::vector<hlslib::ocl::Buffer<Data_t, hlslib::ocl::Access::readWrite>>
      device_buffers;
  for (int i = 0; i < kPX; ++i) {
    for (int j = 0; j < kPY; ++j) {
      auto device_buffer =
          context.MakeBuffer<Data_t, hlslib::ocl::Access::readWrite>(
              2 * kXLocal * kYLocal);
      const std::string suffix("_" + std::to_string(i) + "_" +
                               std::to_string(j));
      kernels.emplace_back(program.MakeKernel("Read" + suffix, device_buffer));
      kernels.emplace_back(program.MakeKernel("Stencil" + suffix));
      kernels.emplace_back(program.MakeKernel("Write" + suffix, device_buffer));
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
  for (auto &k : kernels) {
    futures.emplace_back(k.ExecuteTaskAsync());
  }
  std::cout << "Waiting for kernels to finish...\n" << std::flush;
  for (auto &f : futures) {
    f.wait();
  }

  // Copy back result
  std::cout << "Copying back result...\n" << std::flush;
  int offset = (kT % 2 == 0) ? 0 : kYLocal * kXLocal;
  for (int i = 0; i < kPX; ++i) {
    for (int j = 0; j < kPY; ++j) {
      auto &device_buffer = device_buffers[i * kPY + j];
      auto &host_buffer = host_buffers[i * kPY + j];
      device_buffer.CopyToHost(offset, kYLocal * kXLocal, host_buffer.begin());
    }
  }

  // Run reference implementation
  std::cout << "Running reference implementation...\n" << std::flush;
  Reference(reference);

  const auto result = CombineMemory(host_buffers);

  // Compare result
  for (int i = 0; i < kX; ++i) {
    for (int j = 0; j < kY; ++j) {
      auto diff = std::abs(result[i * kY + j] - reference[i * kY + j]);
      if (diff > 1e-4 * reference[i * kY + j]) {
        std::cerr << "Mismatch found at (" << i << ", " << j
                  << "): " << result[i * kY + j] << " (should be "
                  << reference[i * kY + j] << ").\n";
        return 3;
      }
    }
  }

  std::cout << "Successfully verified result.\n";

  return 0;
}

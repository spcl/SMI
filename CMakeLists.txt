cmake_minimum_required(VERSION 3.0)
project(smi)

# Configuration 
set(SMI_TARGET_BOARD "p520_max_sg280l" CACHE STRING "aoc target board.")
set(SMI_ROUTING_FILE "${CMAKE_SOURCE_DIR}/routing/fpga-15-16.routing" CACHE STRING "File describing routing setup.")
set(SMI_FMAX "300" CACHE STRING "Target Fmax when compiling for hardware.")

# Dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/hlslib/cmake)
find_package(Threads REQUIRED)
find_package(IntelFPGAOpenCL REQUIRED)
find_package(MPI)
include_directories(SYSTEM ${IntelFPGAOpenCL_INCLUDE_DIRS})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/hlslib/include)
include_directories(SYSTEM ${MPI_CXX_INCLUDE_PATH})
set(SMI_LIBS ${CMAKE_THREAD_LIBS_INIT} ${IntelFPGAOpenCL_LIBRARIES} ${MPI_CXX_LIBRARIES})

# Build flags for host executables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -DHLSLIB_INTEL ${MPI_CXX_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")

add_subdirectory(examples)
add_subdirectory(rodinia)

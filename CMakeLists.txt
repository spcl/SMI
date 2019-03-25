cmake_minimum_required(VERSION 3.0)
project(smi)

# Configuration 
set(SMI_TARGET_BOARD "p520_max_sg280l" CACHE STRING "aoc target board.")

# Dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/hlslib/cmake)
find_package(Threads REQUIRED)
find_package(IntelFPGAOpenCL REQUIRED)
include_directories(SYSTEM ${IntelFPGAOpenCL_INCLUDE_DIRS})
set(SMI_LIBS ${CMAKE_THREAD_LIBS_INIT} ${IntelFPGAOpenCL_LIBRARIES})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/hlslib/include)

find_package(MPI)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})

# Build flags for host executables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -DHLSLIB_INTEL")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")

add_subdirectory(examples)
add_subdirectory(rodinia)

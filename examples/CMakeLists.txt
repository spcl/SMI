# Configuration 
set(SMI_STENCIL_SIZE_X 512 CACHE STRING "Vertical size of domain.")
set(SMI_STENCIL_SIZE_Y 512 CACHE STRING "Horizontal size of domain.")
set(SMI_DATATYPE float CACHE STRING "Data type of computations.")
set(SMI_VECTORIZATION_WIDTH 16 CACHE STRING "Width for exploiting vector parallelism.")
set(SMI_STENCIL_NUM_PROCS_X 2 CACHE STRING "Number of processes in X dimension.")
set(SMI_STENCIL_NUM_PROCS_Y 2 CACHE STRING "Number of processes in Y dimension.")
math(EXPR SMI_STENCIL_NUM_PROCS "${SMI_STENCIL_NUM_PROCS_X} * ${SMI_STENCIL_NUM_PROCS_Y}")

if(SMI_DATATYPE STREQUAL "float")
  set(SMI_COMM_DATATYPE "SMI_FLOAT")
else()
  message(FATAL_ERROR "Unsupported data type \"${SMI_DATATYPE}\".")
endif()

configure_file(include/stencil.h.in stencil.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(PythonInterp 3)

add_smi_kernel("stencil_simple" OFF 1 0)
add_smi_executable("stencil_simple")
if(PythonInterp_FOUND)
  add_smi_kernel("stencil_spatial_tiling" ON 1 0)
  add_smi_executable("stencil_spatial_tiling")
  add_smi_kernel("stencil_smi" OFF ${SMI_STENCIL_NUM_PROCS} 4)
  add_smi_executable("stencil_smi")
  add_smi_kernel("stencil_smi_interleaved" OFF ${SMI_STENCIL_NUM_PROCS} 4)
  add_smi_executable("stencil_smi_interleaved")
endif()

# Configuration
set(SMI_STENCIL_SIZE_X 8192 CACHE STRING "Vertical size of domain.")
set(SMI_STENCIL_SIZE_Y 8192 CACHE STRING "Horizontal size of domain.")
set(SMI_DATATYPE float CACHE STRING "Data type of computations.")
set(SMI_VECTORIZATION_WIDTH 16 CACHE STRING "Width for exploiting vector parallelism.")
set(SMI_STENCIL_NUM_PROCS_X 2 CACHE STRING "Number of processes in X dimension.")
set(SMI_STENCIL_NUM_PROCS_Y 4 CACHE STRING "Number of processes in Y dimension.")
math(EXPR SMI_STENCIL_NUM_PROCS "${SMI_STENCIL_NUM_PROCS_X} * ${SMI_STENCIL_NUM_PROCS_Y}")
set(SMI_KMEANS_RANKS 8 CACHE STRING "Number of dimensions for K-means.")
set(SMI_KMEANS_DIMS 64 CACHE STRING "Number of dimensions for K-means.")
set(SMI_KMEANS_CLUSTERS 8 CACHE STRING "Number of clusters to compute.")

if(SMI_DATATYPE STREQUAL "float")
  set(SMI_COMM_DATATYPE "SMI_FLOAT")
else()
  message(FATAL_ERROR "Unsupported data type \"${SMI_DATATYPE}\".")
endif()

configure_file(include/stencil.h.in stencil.h)
configure_file(include/kmeans.h.in kmeans.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(PythonInterp 3)

if(PythonInterp_FOUND)

    smi_target(stencil "${CMAKE_CURRENT_SOURCE_DIR}/kernels/stencil_smi_interleaved.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/stencil_smi_interleaved.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/stencil_smi_interleaved.cl" 8)
    smi_target(kmeans "${CMAKE_CURRENT_SOURCE_DIR}/kernels/kmeans_smi.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/kmeans_smi.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/kmeans_smi.cl" 8)


endif()

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
configure_file(include/fblas.h  fblas.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(PythonInterp 3)

if(PythonInterp_FOUND)
    #stencil
    smi_target(stencil_smi "${CMAKE_CURRENT_SOURCE_DIR}/kernels/stencil_smi.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/stencil_smi.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/stencil_smi.cl" 8)
    #kmeans
    smi_target(kmeans_smi "${CMAKE_CURRENT_SOURCE_DIR}/kernels/kmeans_smi.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/kmeans_smi.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/kmeans_smi.cl" 8)

    #gesummv
    smi_target(gesummv_smi "${CMAKE_CURRENT_SOURCE_DIR}/kernels/gesummv_smi.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/gesummv_smi.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/gesummv_rank0.cl;${CMAKE_CURRENT_SOURCE_DIR}/kernels/gesummv_rank1.cl" 2)
    target_link_libraries(gesummv_smi_host openblas)

    #onchip versions
    fpga_target(gesummv_onchip "${CMAKE_CURRENT_SOURCE_DIR}/host/gesummv_onchip.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/gesummv_onchip.cl" OFF)
    fpga_target(stencil_onchip "${CMAKE_CURRENT_SOURCE_DIR}/host/stencil_onchip.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/stencil_onchip.cl" ON)

endif()

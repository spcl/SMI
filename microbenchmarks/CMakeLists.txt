# Configuration 
set(SMI_STENCIL_SIZE_X 8192 CACHE STRING "Vertical size of domain.")
set(SMI_STENCIL_SIZE_Y 8192 CACHE STRING "Horizontal size of domain.")


include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(PythonInterp 3)

if(PythonInterp_FOUND)
    add_smi_executable("bandwidth_benchmark")
    add_smi_microbenchmarks("${CMAKE_CURRENT_SOURCE_DIR}/kernels/bandwidth.cl" 8 ON)
    add_smi_executable("scatter_benchmark")
    add_smi_microbenchmarks("${CMAKE_CURRENT_SOURCE_DIR}/kernels/scatter.cl" 8 OFF)
endif()

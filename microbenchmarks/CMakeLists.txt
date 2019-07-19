# Configuration 

include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(PythonInterp 3)

if(PythonInterp_FOUND)
	#MPMD benchmarks
    #add_smi_executable("bandwidth_benchmark")
    #add_smi_microbenchmarks("${CMAKE_CURRENT_SOURCE_DIR}/kernels/bandwidth.cl" 8 ON)
    #add_smi_executable("latency_benchmark")
    #add_smi_microbenchmarks("${CMAKE_CURRENT_SOURCE_DIR}/kernels/latency.cl" 8 ON)
    #add_smi_executable("injection_rate_benchmark")
    #add_smi_microbenchmarks("${CMAKE_CURRENT_SOURCE_DIR}/kernels/injection_rate.cl" 8 ON)

    #Collectives (SPMD)
    #add_smi_executable("scatter_benchmark")
    #add_smi_microbenchmarks("${CMAKE_CURRENT_SOURCE_DIR}/kernels/scatter.cl" 8 OFF)
    #add_smi_executable("broadcast_benchmark")
    #add_smi_microbenchmarks("${CMAKE_CURRENT_SOURCE_DIR}/kernels/broadcast.cl" 8 OFF)
    #add_smi_executable("reduce_benchmark")
    #add_smi_microbenchmarks("${CMAKE_CURRENT_SOURCE_DIR}/kernels/reduce.cl" 8 OFF)
    #add_smi_executable("gather_benchmark")
    #add_smi_microbenchmarks("${CMAKE_CURRENT_SOURCE_DIR}/kernels/gather.cl" 8 OFF)

    #others
    #add_smi_executable("multi_collectives_benchmark")
    #add_smi_microbenchmarks("${CMAKE_CURRENT_SOURCE_DIR}/kernels/multi_collectives.cl" 8 OFF)

    #add_smi_microbenchmarks("${CMAKE_CURRENT_SOURCE_DIR}/kernels/s2s_test_0.cl" 8 OFF)
    #add_smi_microbenchmarks("${CMAKE_CURRENT_SOURCE_DIR}/kernels/s2s_test_1.cl" 8 OFF)


    smi_target(broadcast "${CMAKE_CURRENT_SOURCE_DIR}/kernels/broadcast.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/broadcast_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/broadcast.cl" 8)
    smi_target(s2s "${CMAKE_CURRENT_SOURCE_DIR}/kernels/s2s_test.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/bandwidth_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/s2s_test_0.cl;${CMAKE_CURRENT_SOURCE_DIR}/kernels/s2s_test_1.cl" 8)
    smi_target(reduce "${CMAKE_CURRENT_SOURCE_DIR}/kernels/reduce.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/reduce_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/reduce.cl" 8)
    smi_target(bandwidth "${CMAKE_CURRENT_SOURCE_DIR}/kernels/bandwidth.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/bandwidth_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/bandwidth_0.cl;${CMAKE_CURRENT_SOURCE_DIR}/kernels/bandwidth_1.cl" 8)
    smi_target(scatter "${CMAKE_CURRENT_SOURCE_DIR}/kernels/scatter.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/scatter_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/scatter.cl" 8)

endif()

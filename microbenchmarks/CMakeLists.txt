# Configuration 

include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(PythonInterp 3)

if(PythonInterp_FOUND)

    #SPMD
    smi_target(broadcast "${CMAKE_CURRENT_SOURCE_DIR}/kernels/broadcast.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/broadcast_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/broadcast.cl" 8)
    smi_target(s2s "${CMAKE_CURRENT_SOURCE_DIR}/kernels/s2s_test.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/bandwidth_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/s2s_test_0.cl;${CMAKE_CURRENT_SOURCE_DIR}/kernels/s2s_test_1.cl" 8)
    smi_target(reduce "${CMAKE_CURRENT_SOURCE_DIR}/kernels/reduce.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/reduce_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/reduce.cl" 8)
    smi_target(scatter "${CMAKE_CURRENT_SOURCE_DIR}/kernels/scatter.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/scatter_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/scatter.cl" 8)
    smi_target(gather "${CMAKE_CURRENT_SOURCE_DIR}/kernels/gather.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/gather_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/gather.cl" 8)
    smi_target(multi_collectives "${CMAKE_CURRENT_SOURCE_DIR}/kernels/multi_collectives.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/multi_collectives_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/multi_collectives.cl" 8)

    #MPMD
    smi_target(bandwidth "${CMAKE_CURRENT_SOURCE_DIR}/kernels/bandwidth.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/bandwidth_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/bandwidth_0.cl;${CMAKE_CURRENT_SOURCE_DIR}/kernels/bandwidth_1.cl" 8)
    smi_target(injection "${CMAKE_CURRENT_SOURCE_DIR}/kernels/injection_rate.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/injection_rate_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/injection_rate_0.cl;${CMAKE_CURRENT_SOURCE_DIR}/kernels/injection_rate_1.cl" 8)
    smi_target(latency "${CMAKE_CURRENT_SOURCE_DIR}/kernels/latency.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/latency_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/latency_0.cl;${CMAKE_CURRENT_SOURCE_DIR}/kernels/latency_1.cl" 8)

    #Eager evaluation
    smi_target(bandwidth_eager "${CMAKE_CURRENT_SOURCE_DIR}/kernels/bandwidth.json" "${CMAKE_CURRENT_SOURCE_DIR}/host/bandwidth_benchmark.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/kernels/bandwidth_0.cl;${CMAKE_CURRENT_SOURCE_DIR}/kernels/bandwidth_1.cl" 8 8 8 OFF)

endif()


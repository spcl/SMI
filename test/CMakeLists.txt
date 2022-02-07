# Download and unpack googletest at configure time

configure_file(${PROJECT_SOURCE_DIR}/CMakeGTEST.txt.in googletest-download/CMakeLists.txt)


execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

#include the gtest header directories
include_directories("${gtest_SOURCE_DIR}/include")

# Now simply link against gtest or gtest_main as needed. Eg
link_libraries(gtest_main)

#p2p
smi_target(test_p2p "${CMAKE_CURRENT_SOURCE_DIR}/p2p/p2p.json" "${CMAKE_CURRENT_SOURCE_DIR}/p2p/test_p2p.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/p2p/p2p_rank0.cl;${CMAKE_CURRENT_SOURCE_DIR}/p2p/p2p_rank1.cl" 8)
target_link_libraries(test_p2p_host gtest)

add_test(
   NAME p2p
   COMMAND  env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 test_p2p_host
   WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_p2p/"
 )


#broadcast
smi_target(test_broadcast "${CMAKE_CURRENT_SOURCE_DIR}/broadcast/broadcast.json" "${CMAKE_CURRENT_SOURCE_DIR}/broadcast/test_broadcast.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/broadcast/broadcast.cl" 8)
target_link_libraries(test_broadcast_host gtest)

add_test(
  NAME broadcast
  COMMAND  env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 test_broadcast_host
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_broadcast/"
)

#reduce
smi_target(test_reduce "${CMAKE_CURRENT_SOURCE_DIR}/reduce/reduce.json" "${CMAKE_CURRENT_SOURCE_DIR}/reduce/test_reduce.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/reduce/reduce.cl" 8)
target_link_libraries(test_reduce_host gtest)

add_test(
   NAME reduce
   COMMAND  env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 test_reduce_host
   WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_reduce/"
 )


smi_target(test_scatter "${CMAKE_CURRENT_SOURCE_DIR}/scatter/scatter.json" "${CMAKE_CURRENT_SOURCE_DIR}/scatter/test_scatter.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/scatter/scatter.cl" 8)
target_link_libraries(test_scatter_host gtest)

add_test(
   NAME scatter
   COMMAND  env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 test_scatter_host
   WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_scatter/"
 )

smi_target(test_gather "${CMAKE_CURRENT_SOURCE_DIR}/gather/gather.json" "${CMAKE_CURRENT_SOURCE_DIR}/gather/test_gather.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/gather/gather.cl" 8)
target_link_libraries(test_gather_host gtest)

add_test(
   NAME gather
   COMMAND  env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 test_gather_host
   WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_gather/"
 )


smi_target(test_mixed "${CMAKE_CURRENT_SOURCE_DIR}/mixed/mixed.json" "${CMAKE_CURRENT_SOURCE_DIR}/mixed/test_mixed.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/mixed/mixed.cl" 8)
target_link_libraries(test_mixed_host gtest)

add_test(
   NAME mixed
   COMMAND  env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 test_mixed_host
   WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_mixed/"
 )

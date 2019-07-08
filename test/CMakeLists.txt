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

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories("${gtest_SOURCE_DIR}/include")
endif()

# Now simply link against gtest or gtest_main as needed. Eg
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#p2p
add_smi_test("${CMAKE_CURRENT_SOURCE_DIR}/p2p/p2p.cl" 8 ON)
add_smi_test_executable("${CMAKE_CURRENT_SOURCE_DIR}/p2p/test_p2p.cpp")
add_test(
   NAME p2p
   COMMAND  env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 "./test_p2p.exe" "./p2p_emulator_<rank>.aocx"
 )

#broadcast
add_smi_test("${CMAKE_CURRENT_SOURCE_DIR}/broadcast/broadcast.cl" 8 OFF)
add_smi_test_executable("${CMAKE_CURRENT_SOURCE_DIR}/broadcast/test_broadcast.cpp")
add_test(
   NAME broadcast
   COMMAND  env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 "./test_broadcast.exe" "./broadcast_emulator_<rank>.aocx"
 )


 #gather
add_smi_test("${CMAKE_CURRENT_SOURCE_DIR}/gather/gather.cl" 8 OFF)
add_smi_test_executable("${CMAKE_CURRENT_SOURCE_DIR}/gather/test_gather.cpp")
add_test(
   NAME gather
   COMMAND  env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 "./test_gather.exe" "./gather_emulator_<rank>.aocx"
 )

 #reduce
add_smi_test("${CMAKE_CURRENT_SOURCE_DIR}/reduce/reduce.cl" 8 OFF)
add_smi_test_executable("${CMAKE_CURRENT_SOURCE_DIR}/reduce/test_reduce.cpp")
add_test(
   NAME reduce
   COMMAND  env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 "./test_reduce.exe" "./reduce_emulator_<rank>.aocx"
 )


#scatter
add_smi_test("${CMAKE_CURRENT_SOURCE_DIR}/scatter/scatter.cl" 8 OFF)
add_smi_test_executable("${CMAKE_CURRENT_SOURCE_DIR}/scatter/test_scatter.cpp")
add_test(
   NAME scatter
   COMMAND  env  CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=8 mpirun -np 8 "./test_scatter.exe" "./scatter_emulator_<rank>.aocx"
 )

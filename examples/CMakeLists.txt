# Configuration 
set(SMI_STENCIL_SIZE_X 512 CACHE STRING "Vertical size of domain.")
set(SMI_STENCIL_SIZE_Y 512 CACHE STRING "Horizontal size of domain.")
set(SMI_DATATYPE float CACHE STRING "Data type of computations.")
set(SMI_VECTORIZATION_WIDTH 16 CACHE STRING "Width for exploiting vector parallelism.")
set(SMI_STENCIL_NUM_PROCS_X 2 CACHE STRING "Number of processes in X dimension.")
set(SMI_STENCIL_NUM_PROCS_Y 2 CACHE STRING "Number of processes in Y dimension.")

configure_file(include/stencil.h.in stencil.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(PythonInterp)

# All targets related to each kernel 
function(opencl_target KERNEL_NAME USE_CODEGEN)
  if(USE_CODEGEN)
    add_custom_target(generate_${KERNEL_NAME} ALL
      COMMAND ${PYTHON_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.py 
      ${SMI_STENCIL_NUM_PROCS_X} ${SMI_STENCIL_NUM_PROCS_Y}
      ${CMAKE_CURRENT_SOURCE_DIR}/kernels ${CMAKE_CURRENT_BINARY_DIR})
    set(KERNEL_FILE ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}.cl)
  else()
    set(KERNEL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.cl)
  endif()
  set(AOC_COMMAND
    -I${CMAKE_SOURCE_DIR}/include
    -I${CMAKE_SOURCE_DIR}/hlslib/include
    -I${CMAKE_CURRENT_SOURCE_DIR}/include
    -I${CMAKE_BINARY_DIR}
    -I${CMAKE_CURRENT_BINARY_DIR}
    -fp-relaxed
    -cl-no-signed-zeros
    -cl-fast-relaxed-math
    -cl-single-precision-constant
    -board=${SMI_TARGET_BOARD}
    ${KERNEL_FILE})
  add_custom_target(build_${KERNEL_NAME}_report
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} 
    -rtl -report)
  add_custom_target(build_${KERNEL_NAME}_emulator
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} -march=emulator
    -emulator-channel-depth-model=strict
    -o ${KERNEL_NAME}_emulator.aocx)
  add_custom_target(build_${KERNEL_NAME}_hardware
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} -o ${KERNEL_NAME}_hardware.aocx)
  if(USE_CODEGEN)
    add_dependencies(build_${KERNEL_NAME}_report generate_${KERNEL_NAME})
    add_dependencies(build_${KERNEL_NAME}_emulator generate_${KERNEL_NAME})
    add_dependencies(build_${KERNEL_NAME}_hardware generate_${KERNEL_NAME})
  endif()
  add_executable(${KERNEL_NAME}.exe ${CMAKE_CURRENT_SOURCE_DIR}/host/${KERNEL_NAME}.cpp)
  target_link_libraries(${KERNEL_NAME}.exe ${IntelFPGAOpenCL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  # Run targets don't work anymore when we need to pass arguments
  # add_custom_target(run_${KERNEL_NAME}_hardware COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}.exe hardware)
  # add_custom_target(run_${KERNEL_NAME}_emulator COMMAND CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=1 ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}.exe emulator)
  set(SMI_STENCIL_MAKE_TARGET build_${KERNEL_NAME}_hardware)
  mark_as_advanced(SMI_STENCIL_MAKE_TARGET)
  configure_file(build.sh.in build_${KERNEL_NAME}.sh)
endfunction()

opencl_target("stencil_simple" OFF)
if(PythonInterp_FOUND)
  opencl_target("stencil_spatial_tiling" ON)
endif()

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

# All targets related to each kernel 
function(opencl_target KERNEL_NAME USE_CODEGEN NUM_TAGS)
  if(USE_CODEGEN)
    add_custom_target(generate_${KERNEL_NAME}
      COMMAND ${PYTHON_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.py 
      ${SMI_STENCIL_NUM_PROCS_X} ${SMI_STENCIL_NUM_PROCS_Y}
      ${CMAKE_CURRENT_SOURCE_DIR}/kernels ${CMAKE_CURRENT_BINARY_DIR})
    set(KERNEL_FILE ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}.cl)
  else()
    set(KERNEL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.cl)
  endif()
  if(NUM_TAGS GREATER 0)
    add_custom_target(generate_${KERNEL_NAME}_routing
        COMMAND ${PYTHON_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/routing/main.py build ${SMI_ROUTING_FILE}
        ${KERNEL_NAME}_routing ${NUM_TAGS})
    if(USE_CODEGEN)
      add_dependencies(generate_${KERNEL_NAME} generate_${KERNEL_NAME}_routing)
    endif()
  endif()
  set(AOC_COMMAND
    -I${CMAKE_SOURCE_DIR}/include
    -I${CMAKE_SOURCE_DIR}/hlslib/include
    -I${CMAKE_CURRENT_SOURCE_DIR}/include
    -I${CMAKE_BINARY_DIR}
    -I${CMAKE_CURRENT_BINARY_DIR}
    -I${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}_routing
    -fp-relaxed
    -cl-no-signed-zeros
    -cl-fast-relaxed-math
    -cl-single-precision-constant
    -no-interleaving=default
    -board=${SMI_TARGET_BOARD}
    ${KERNEL_FILE})
  add_custom_target(build_${KERNEL_NAME}_report
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} 
    -rtl -report)
  if(NUM_TAGS EQUAL 0)
    add_custom_target(build_${KERNEL_NAME}_emulator
      COMMAND ${IntelFPGAOpenCL_AOC} 
      ${AOC_COMMAND} -march=emulator
      -emulator-channel-depth-model=strict
      -o ${KERNEL_NAME}_emulator.aocx)
  else()
    # To emulate an MPI-like setup, we need to generate a separate emulation
    # kernel for each rank
    add_custom_target(build_${KERNEL_NAME}_emulator)
    math(EXPR LOOP_END "${NUM_TAGS} - 1")
    foreach(SMI_EMULATION_RANK RANGE 0 ${LOOP_END})
      add_custom_target(build_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}
        COMMAND ${IntelFPGAOpenCL_AOC} 
        ${AOC_COMMAND} -march=emulator
        -DSMI_EMULATION_RANK=${SMI_EMULATION_RANK}
        -emulator-channel-depth-model=strict
        -o ${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}.aocx)
      add_dependencies(build_${KERNEL_NAME}_emulator build_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK})
    endforeach()
  endif()
  add_custom_target(build_${KERNEL_NAME}_hardware
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} -o ${KERNEL_NAME}_hardware.aocx)
  if(USE_CODEGEN)
    add_dependencies(build_${KERNEL_NAME}_report generate_${KERNEL_NAME})
    add_dependencies(build_${KERNEL_NAME}_emulator generate_${KERNEL_NAME})
    add_dependencies(build_${KERNEL_NAME}_hardware generate_${KERNEL_NAME})
  endif()
  if(NUM_TAGS GREATER 0)
    add_dependencies(build_${KERNEL_NAME}_report generate_${KERNEL_NAME}_routing)
    add_dependencies(build_${KERNEL_NAME}_emulator generate_${KERNEL_NAME}_routing)
    add_dependencies(build_${KERNEL_NAME}_hardware generate_${KERNEL_NAME}_routing)
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

opencl_target("stencil_simple" OFF 0)
if(PythonInterp_FOUND)
  opencl_target("stencil_spatial_tiling" ON 0)
  opencl_target("stencil_smi" OFF ${SMI_STENCIL_NUM_PROCS})
endif()

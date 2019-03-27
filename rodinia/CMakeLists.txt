# Configuration 
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-DALTERA)

# All targets related to each kernel 
function(opencl_target KERNEL_NAME KERNEL_FILE HOST_FILES)
  set(KERNEL_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${KERNEL_NAME})
  set(KERNEL_PATH ${KERNEL_FOLDER}/kernel/${KERNEL_FILE}.cl)
  set(AOC_COMMAND
    -I${CMAKE_SOURCE_DIR}/include
    -I${CMAKE_SOURCE_DIR}/hlslib/include
    -I${CMAKE_CURRENT_SOURCE_DIR}
    -I${KERNEL_FOLDER}
    -I${KERNEL_FOLDER}/util
    -fp-relaxed
    -cl-no-signed-zeros
    -cl-fast-relaxed-math
    -cl-single-precision-constant
    -no-interleaving=default
    -board=${SMI_TARGET_BOARD}
    ${KERNEL_PATH})
  add_custom_target(build_${KERNEL_FILE}_report
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} 
    -rtl -report)
  add_custom_target(build_${KERNEL_FILE}_emulator
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} -march=emulator
    -emulator-channel-depth-model=strict
    -o ${KERNEL_FILE}_emulator.aocx)
  add_custom_target(build_${KERNEL_FILE}_hardware
    COMMAND ${IntelFPGAOpenCL_AOC} 
    -report ${AOC_COMMAND} -o ${KERNEL_FILE}_hardware.aocx)
  if(USE_CODEGEN)
    add_dependencies(build_${KERNEL_FILE}_report generate_${KERNEL_FILE})
    add_dependencies(build_${KERNEL_FILE}_emulator generate_${KERNEL_FILE})
    add_dependencies(build_${KERNEL_FILE}_hardware generate_${KERNEL_FILE})
  endif()
  string(REPLACE " " ";" HOST_FILES "${HOST_FILES}")
  string(REGEX REPLACE "([^;]+)" "${KERNEL_FOLDER}/\\1"
         HOST_FILES "${HOST_FILES}")
  add_executable(${KERNEL_NAME}.exe ${HOST_FILES})
  target_link_libraries(${KERNEL_NAME}.exe ${IntelFPGAOpenCL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} m)
  target_include_directories(${KERNEL_NAME}.exe PRIVATE ${KERNEL_FOLDER} ${KERNEL_FOLDER}/util)
endfunction()

opencl_target("srad" "srad_kernel_v5" "main.c util/graphics/graphics.c util/graphics/resize.c kernel/kernel_gpu_opencl_wrapper.c util/opencl/opencl.c")

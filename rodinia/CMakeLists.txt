# Configuration 
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-DALTERA -DNO_INTERLEAVE)

# All targets related to each kernel 
function(opencl_target KERNEL_FOLDER_NAME KERNEL_FILE HOST_FILES)
  set(KERNEL_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${KERNEL_FOLDER_NAME})
  get_filename_component(KERNEL_NAME ${KERNEL_FILE} NAME_WE)
  set(AOC_COMMAND
    -I${CMAKE_CURRENT_SOURCE_DIR}
    -I${KERNEL_FOLDER}
    -I${KERNEL_FOLDER}/util
    -fp-relaxed
    -cl-no-signed-zeros
    -cl-fast-relaxed-math
    -cl-single-precision-constant
    -no-interleaving=default
    -board=${SMI_TARGET_BOARD}
    ${KERNEL_FOLDER}/${KERNEL_FILE})
  add_custom_target(build_rodinia_${KERNEL_NAME}_report
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} 
    -rtl -report)
  add_custom_target(build_rodinia_${KERNEL_NAME}_emulator
    COMMAND ${IntelFPGAOpenCL_AOC} 
    ${AOC_COMMAND} -march=emulator
    -emulator-channel-depth-model=strict
    -o ${KERNEL_NAME}_emulator.aocx)
  add_custom_target(build_rodinia_${KERNEL_NAME}_hardware
    COMMAND ${IntelFPGAOpenCL_AOC} 
    -report ${AOC_COMMAND} -o ${KERNEL_NAME}_hardware.aocx)
  string(REPLACE " " ";" HOST_FILES "${HOST_FILES}")
  string(REGEX REPLACE "([^;]+)" "${KERNEL_FOLDER}/\\1"
         HOST_FILES "${HOST_FILES}")
  add_executable(${KERNEL_FOLDER_NAME}.exe ${HOST_FILES})
  target_link_libraries(${KERNEL_FOLDER_NAME}.exe ${IntelFPGAOpenCL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} m)
  target_include_directories(${KERNEL_FOLDER_NAME}.exe PRIVATE ${KERNEL_FOLDER} ${KERNEL_FOLDER}/util)
endfunction()

opencl_target("srad" "kernel/srad_kernel_v5.cl" "main.c util/graphics/graphics.c util/graphics/resize.c kernel/kernel_gpu_opencl_wrapper.c util/opencl/opencl.c")
opencl_target("pathfinder" "pathfinder_kernel_v5.cl" "main.cpp OpenCL.cpp")

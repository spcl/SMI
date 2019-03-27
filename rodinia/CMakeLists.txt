# Configuration 
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-DALTERA -DNO_INTERLEAVE -DPROFILE -DBENCHMARK)

find_package(OpenMP)
set(SMI_RODINIA_LIBS ${IntelFPGAOpenCL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
    m ${OpenMP_CXX_LIBRARIES})

# All targets related to each kernel 
function(add_opencl_executable KERNEL_FOLDER_NAME HOST_FILES)
  set(KERNEL_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${KERNEL_FOLDER_NAME})
  string(REPLACE " " ";" HOST_FILES "${HOST_FILES}")
  string(REGEX REPLACE "([^;]+)" "${KERNEL_FOLDER}/\\1"
         HOST_FILES "${HOST_FILES}")
  add_executable(${KERNEL_FOLDER_NAME}.exe ${HOST_FILES})
  target_link_libraries(${KERNEL_FOLDER_NAME}.exe ${SMI_RODINIA_LIBS})
  target_include_directories(${KERNEL_FOLDER_NAME}.exe PRIVATE ${KERNEL_FOLDER} ${KERNEL_FOLDER}/util)
  target_compile_options(${KERNEL_FOLDER_NAME}.exe PRIVATE "${OpenMP_CXX_FLAGS}")
endfunction()

function(add_opencl_kernel KERNEL_FOLDER_NAME KERNEL_FILE)
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
endfunction()

add_opencl_executable("srad" "main.c util/graphics/graphics.c util/graphics/resize.c kernel/kernel_gpu_opencl_wrapper.c util/opencl/opencl.c")
add_opencl_kernel("srad" "kernel/srad_kernel_v5.cl")
add_opencl_executable("pathfinder" "main.cpp OpenCL.cpp")
add_opencl_kernel("pathfinder" "pathfinder_kernel_v5.cl")
add_opencl_kernel("pathfinder" "pathfinder_kernel_v3.cl")
add_opencl_executable("nw" "nw.cpp")
add_opencl_kernel("nw" "nw_kernel_v5.cl")
add_opencl_executable("hotspot" "hotspot.c OpenCL_helper_library.c")
add_opencl_kernel("hotspot" "hotspot_kernel_v5.cl")
add_opencl_executable("lud" "ocl/lud.cpp common/common.c")
add_opencl_kernel("lud" "ocl/lud_kernel_v5.cl")
add_opencl_executable("cfd" "euler3d.cpp CLHelper2.cpp host_common.cpp")
add_opencl_kernel("cfd" "euler3d_kernel_v8.cl")

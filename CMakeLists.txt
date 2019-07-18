cmake_minimum_required(VERSION 3.0)
project(smi)

# Configuration
set(SMI_TARGET_BOARD "p520_max_sg280l" CACHE STRING "aoc target board.")
set(SMI_ROUTING_FILE "${CMAKE_SOURCE_DIR}/routing/fpga-06-07-08-09.json" CACHE STRING "File describing routing setup.")
set(TEST_ROUTING_FILE "${CMAKE_SOURCE_DIR}/test/network_topologies/8FPGAs-2D-torus.routing" CACHE STRING "File describing routing setup for testing.")

set(SMI_REWRITER "${CMAKE_BINARY_DIR}/source-rewriter/rewriter")

set(SMI_FMAX "300" CACHE STRING "Target Fmax when compiling for hardware.")
set(SMI_DEVICES_PER_NODE 2 CACHE STRING "Number of FPGA devices per node.")

option (ENABLE_TESTS "Enables testing" OFF)

# Dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/hlslib/cmake)
find_package(Threads REQUIRED)
find_package(IntelFPGAOpenCL REQUIRED)
find_package(MPI REQUIRED)
include_directories(SYSTEM ${IntelFPGAOpenCL_INCLUDE_DIRS})
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/hlslib/include)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/include)

include_directories(SYSTEM ${MPI_CXX_INCLUDE_PATH})
set(SMI_LIBS ${CMAKE_THREAD_LIBS_INIT} ${IntelFPGAOpenCL_LIBRARIES} ${MPI_CXX_LIBRARIES})
set(GTEST_LIBS "-lgtest")

# Build flags for host executables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -DHLSLIB_INTEL ${MPI_CXX_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")

# Executable for launching OpenCL kernels
function(add_smi_executable KERNEL_NAME)
    add_executable(${KERNEL_NAME}.exe ${CMAKE_CURRENT_SOURCE_DIR}/host/${KERNEL_NAME}.cpp)
    target_link_libraries(${KERNEL_NAME}.exe ${SMI_LIBS})
endfunction()

# All targets related to building OpenCL kernels
function(add_smi_kernel KERNEL_PATH USE_CODEGEN NUM_RANKS NUM_TAGS CUSTOM_ROUTING)
    get_filename_component(KERNEL_NAME ${KERNEL_PATH} NAME_WE)
    if(USE_CODEGEN)
        add_custom_target(generate_${KERNEL_NAME}
            COMMAND ${PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.py
            ${SMI_STENCIL_NUM_PROCS_X} ${SMI_STENCIL_NUM_PROCS_Y}
            ${CMAKE_CURRENT_SOURCE_DIR}/kernels ${CMAKE_CURRENT_BINARY_DIR})
        set(KERNEL_FILE ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}.cl)
    else()
        set(KERNEL_FILE ${KERNEL_PATH})
    endif()
    if(NUM_RANKS GREATER 1 AND NOT CUSTOM_ROUTING)
        add_custom_target(generate_${KERNEL_NAME}_routing
            COMMAND ${PYTHON_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/routing/main.py build ${CMAKE_CURRENT_SOURCE_DIR}/kernels/program_${KERNEL_NAME}.json  ${SMI_ROUTING_FILE}
            ${KERNEL_NAME}_routing)
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
        -fmax=${SMI_FMAX}
        -board=${SMI_TARGET_BOARD}
        ${KERNEL_FILE})
    add_custom_target(build_${KERNEL_NAME}_report
        COMMAND ${IntelFPGAOpenCL_AOC}
        ${AOC_COMMAND}
        -rtl -report)
    if(NUM_RANKS EQUAL 1)
        add_custom_target(build_${KERNEL_NAME}_emulator
            COMMAND ${IntelFPGAOpenCL_AOC}
            ${AOC_COMMAND} -march=emulator
            -emulator-channel-depth-model=strict
            -o ${KERNEL_NAME}_emulator.aocx)
    else()
        # To emulate an MPI-like setup, we need to generate a separate emulation
        # kernel for each rank
        add_custom_target(build_${KERNEL_NAME}_emulator)
        math(EXPR LOOP_END "${NUM_RANKS} - 1")
        foreach(SMI_EMULATION_RANK RANGE 0 ${LOOP_END})
            add_custom_target(build_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}
                COMMAND ${IntelFPGAOpenCL_AOC}
                ${AOC_COMMAND} -march=emulator
                -DSMI_EMULATION_RANK=${SMI_EMULATION_RANK}
                -emulator-channel-depth-model=strict
                -o ${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}.aocx)
            add_dependencies(build_${KERNEL_NAME}_emulator
                build_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK})
        endforeach()
        add_dependencies(build_${KERNEL_NAME}_emulator_0
            generate_${KERNEL_NAME}_routing)
    endif()
    add_custom_target(build_${KERNEL_NAME}_hardware
        COMMAND ${IntelFPGAOpenCL_AOC}
        ${AOC_COMMAND} -o ${KERNEL_NAME}_hardware.aocx)
    if(USE_CODEGEN)
        add_dependencies(build_${KERNEL_NAME}_report generate_${KERNEL_NAME})
        add_dependencies(build_${KERNEL_NAME}_emulator generate_${KERNEL_NAME})
        add_dependencies(build_${KERNEL_NAME}_hardware generate_${KERNEL_NAME})
    endif()
    if(NUM_RANKS GREATER 1)
        add_dependencies(build_${KERNEL_NAME}_report generate_${KERNEL_NAME}_routing)
        add_dependencies(build_${KERNEL_NAME}_emulator generate_${KERNEL_NAME}_routing)
        add_dependencies(build_${KERNEL_NAME}_hardware generate_${KERNEL_NAME}_routing)
    endif()
    # Run targets don't work anymore when we need to pass arguments
    # add_custom_target(run_${KERNEL_NAME}_hardware COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}.exe hardware)
    # add_custom_target(run_${KERNEL_NAME}_emulator COMMAND CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=1 ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}.exe emulator)
    set(SMI_STENCIL_MAKE_TARGET build_${KERNEL_NAME}_hardware)
    mark_as_advanced(SMI_STENCIL_MAKE_TARGET)
    configure_file(${CMAKE_SOURCE_DIR}/build.sh.in build_${KERNEL_NAME}.sh)
endfunction()

## Targets related to microbenchmarks
# NUM_RANKS is used for generating the right emulations binaries
# MPMD used for the benchmarks described as MPMD programs
function(add_smi_microbenchmarks KERNEL_PATH NUM_RANKS MPMD)
    get_filename_component(KERNEL_NAME ${KERNEL_PATH} NAME_WE)
    file(RELATIVE_PATH KERNEL_FILE ${CMAKE_CURRENT_SOURCE_DIR} ${KERNEL_PATH})

    #code-gen

    set(SMI_GENERATED_HOST ${CMAKE_CURRENT_BINARY_DIR}/smi-generated-host.c)
    set(SMI_GENERATED_DEVICE ${CMAKE_CURRENT_BINARY_DIR}/smi-generated-device.cl)

    # user kernel file, modified by codegen in the build directory
    set(SMI_DEVICE_KERNEL ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_FILE})

    #set(AOC_SRC_FILES "${SMI_DEVICE_KERNEL}" "${SMI_GENERATED_DEVICE}")
    set(AOC_SRC_FILES "${SMI_DEVICE_KERNEL}")
    add_custom_target(generate_${KERNEL_NAME}_codegen ALL
        COMMAND python
        ${CMAKE_SOURCE_DIR}/routing/main.py codegen
            --include '${CMAKE_SOURCE_DIR}/include'
            ${SMI_ROUTING_FILE}
            ${SMI_REWRITER}
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}
            ${SMI_GENERATED_HOST}
            ${SMI_GENERATED_DEVICE}
            ${CMAKE_CURRENT_BINARY_DIR}/pg1.json
            ${KERNEL_FILE})
    add_dependencies(generate_${KERNEL_NAME}_codegen rewriter)

    add_custom_target(generate_${KERNEL_NAME}_routing ALL
            COMMAND python
            ${CMAKE_SOURCE_DIR}/routing/main.py route
            ${SMI_ROUTING_FILE}
            ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(generate_${KERNEL_NAME}_routing generate_${KERNEL_NAME}_codegen)

    #add_dependencies(generate_${KERNEL_NAME} generate_${KERNEL_NAME}_routing)
    set(AOC_COMMAND
        -I${CMAKE_SOURCE_DIR}/include
        -I${CMAKE_SOURCE_DIR}/hlslib/include
        -I${CMAKE_CURRENT_SOURCE_DIR}/include
        -I${CMAKE_BINARY_DIR}
        -I${CMAKE_CURRENT_BINARY_DIR}
        -fp-relaxed
        -no-interleaving=default
        -fmax=${SMI_FMAX}
        -board=${SMI_TARGET_BOARD}
    )

    if(MPMD)
        add_custom_target(build_${KERNEL_NAME}_report
            COMMAND ${IntelFPGAOpenCL_AOC}
            ${AOC_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}_rank0.cl  -rtl -report
            COMMAND ${IntelFPGAOpenCL_AOC} ${AOC_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}_rank1.cl  -rtl -report)
        # To emulate an MPI-like setup, we need to generate a separate emulation
        # kernel for each rank
        add_custom_target(build_${KERNEL_NAME}_emulator )
        math(EXPR LOOP_END "${NUM_RANKS} - 1")
        foreach(SMI_EMULATION_RANK RANGE 0 ${LOOP_END} 1)
            if(SMI_EMULATION_RANK EQUAL 0)
                set(KERNEL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}_rank0.cl)
            else()
                set(KERNEL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}_rank1.cl)
            endif()
            add_custom_target(build_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}
                COMMAND ${IntelFPGAOpenCL_AOC}
                ${AOC_COMMAND} ${KERNEL_FILE} -march=emulator
                -DSMI_EMULATION_RANK=${SMI_EMULATION_RANK}
                -emulator-channel-depth-model=strict
                -o ${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}.aocx)
            add_dependencies(build_${KERNEL_NAME}_emulator
                build_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK})
            add_dependencies(build_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}
                generate_${KERNEL_NAME}_routing)
        endforeach()
        add_custom_target(build_${KERNEL_NAME}_hardware
            COMMAND ${IntelFPGAOpenCL_AOC}
            ${AOC_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}_rank0.cl -o ${KERNEL_NAME}_rank0_hardware.aocx
            COMMAND ${IntelFPGAOpenCL_AOC} ${AOC_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}_rank1.cl -o ${KERNEL_NAME}_rank1_hardware.aocx;)
    else()
        add_custom_target(build_${KERNEL_NAME}_report
            COMMAND ${IntelFPGAOpenCL_AOC}
            ${AOC_COMMAND} ${AOC_SRC_FILES} -rtl -report)
        # To emulate an MPI-like setup, we need to generate a separate emulation
        # kernel for each rank
        add_custom_target(build_${KERNEL_NAME}_emulator )
        math(EXPR LOOP_END "${NUM_RANKS} - 1")
        foreach(SMI_EMULATION_RANK RANGE 0 ${LOOP_END} 1)
            add_custom_target(build_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}
                COMMAND ${IntelFPGAOpenCL_AOC}
                ${AOC_COMMAND} ${AOC_SRC_FILES} -march=emulator
                -DSMI_EMULATION_RANK=${SMI_EMULATION_RANK}
                -emulator-channel-depth-model=strict
                -o ${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}.aocx)
            add_dependencies(build_${KERNEL_NAME}_emulator
                build_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK})
            add_dependencies(build_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}
                generate_${KERNEL_NAME}_routing)
        endforeach()
        add_custom_target(build_${KERNEL_NAME}_hardware
            COMMAND ${IntelFPGAOpenCL_AOC}
            ${AOC_COMMAND} ${AOC_SRC_FILES} -o ${KERNEL_NAME}_hardware.aocx)
    endif()
    add_dependencies(build_${KERNEL_NAME}_report generate_${KERNEL_NAME}_routing)
    add_dependencies(build_${KERNEL_NAME}_emulator generate_${KERNEL_NAME}_routing)
    add_dependencies(build_${KERNEL_NAME}_hardware generate_${KERNEL_NAME}_routing)
endfunction()

function(smi_target TARGET_NAME CONNECTION_FILE HOST_SOURCE KERNELS NUM_RANKS)
    set(SMI_SCRIPT ${CMAKE_SOURCE_DIR}/routing/main.py)

    set(PROGRAM_METADATA)           # list of produced JSON metadata (one per program)
    set(KERNEL_TARGETS)             # list of targets (one per program)
    set(FPGA_SOURCES)               # list of transformed user device files (one per program)
    set(FPGA_GENERATED_SOURCES)     # list of generated device files (one per program)

    set(WORKDIR ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME})
    file(MAKE_DIRECTORY ${WORKDIR})

    set(AOC_COMMAND
        -I${CMAKE_SOURCE_DIR}/include
        -I${CMAKE_SOURCE_DIR}/hlslib/include
        -fp-relaxed
        -no-interleaving=default
        -fmax=${SMI_FMAX}
        -board=${SMI_TARGET_BOARD}
    )

    # codegen and compile all programs
    foreach(KERNEL IN ITEMS ${KERNELS})
        get_filename_component(KERNEL_NAME ${KERNEL} NAME_WE)
        get_filename_component(KERNEL_NAME_EXT ${KERNEL} NAME)
        get_filename_component(KERNEL_SRC_DIR ${KERNEL} DIRECTORY)
        set(KERNEL_BIN_DIR ${WORKDIR}/${KERNEL_NAME})
        set(KERNEL_GENERATED_PATH ${KERNEL_BIN_DIR}/${KERNEL_NAME_EXT})
        set(SMI_GENERATED_PATH ${KERNEL_BIN_DIR}/smi-generated-device.cl)
        file(RELATIVE_PATH KERNEL_GENERATED_RELATIVE ${KERNEL_SRC_DIR} ${KERNEL})
        set(KERNEL_TARGET ${TARGET_NAME}_${KERNEL_NAME}_codegen_device)
        set(KERNEL_METADATA ${KERNEL_NAME}.json)
        list(APPEND PROGRAM_METADATA ${WORKDIR}/${KERNEL_METADATA})
        list(APPEND KERNEL_TARGETS ${KERNEL_TARGET})
        list(APPEND FPGA_SOURCES ${KERNEL_GENERATED_PATH})
        list(APPEND FPGA_GENERATED_SOURCES ${SMI_GENERATED_PATH})

        add_custom_target(${KERNEL_TARGET}
            COMMAND python
                ${SMI_SCRIPT} codegen-device
                --include '${CMAKE_SOURCE_DIR}/include'
                ${CONNECTION_FILE}
                ${SMI_REWRITER}
                ${KERNEL_SRC_DIR}
                ${KERNEL_BIN_DIR}
                ${SMI_GENERATED_PATH}
                ${KERNEL_METADATA}
                ${KERNEL_GENERATED_RELATIVE}
            WORKING_DIRECTORY ${WORKDIR}
        )
        add_dependencies(${KERNEL_TARGET} rewriter)

        # compile FPGA code
        set(FPGA_SRC_FILES "${SMI_GENERATED_PATH};${KERNEL_GENERATED_PATH}")

        # generate report
        set(FPGA_REPORT_TARGET ${TARGET_NAME}_${KERNEL_NAME}_aoc_report)
        add_custom_target(${FPGA_REPORT_TARGET}
                COMMAND ${IntelFPGAOpenCL_AOC} ${AOC_COMMAND} ${FPGA_SRC_FILES} -rtl -report
                WORKING_DIRECTORY ${KERNEL_BIN_DIR}
        )
        add_dependencies(${FPGA_REPORT_TARGET} ${KERNEL_TARGET})

        # build hardware
        set(FPGA_BUILD_TARGET ${TARGET_NAME}_${KERNEL_NAME}_aoc_build)
        add_custom_target(${FPGA_BUILD_TARGET}
            COMMAND ${IntelFPGAOpenCL_AOC} ${AOC_COMMAND} ${FPGA_SRC_FILES} -o ${KERNEL_NAME}_hardware.aocx
            WORKING_DIRECTORY ${KERNEL_BIN_DIR}
        )
        add_dependencies(${FPGA_BUILD_TARGET} ${KERNEL_TARGET})
    endforeach()

    # generate routing
    set(ROUTING_TARGET ${TARGET_NAME}_${KERNEL_NAME}_routing)
    add_custom_target(${ROUTING_TARGET}
            COMMAND python
                ${SMI_SCRIPT} route
                ${CONNECTION_FILE}
                ${WORKDIR}/smi-routes
                ${PROGRAM_METADATA}
            WORKING_DIRECTORY ${WORKDIR}
    )
    foreach(TARGET IN ITEMS ${KERNEL_TARGETS})
        add_dependencies(${ROUTING_TARGET} ${TARGET})
    endforeach()

    # generate host code
    set(HOST_GENERATED_TARGET ${TARGET_NAME}_${KERNEL_NAME}_codegen_host)
    set(SMI_HOST_GENERATED_PATH ${WORKDIR}/smi-generated-host.c)
    add_custom_target(${HOST_GENERATED_TARGET}
            COMMAND python
                ${SMI_SCRIPT} codegen-host
                ${SMI_HOST_GENERATED_PATH}
                ${PROGRAM_METADATA}
            WORKING_DIRECTORY ${WORKDIR}
    )
    add_dependencies(${HOST_GENERATED_TARGET} ${ROUTING_TARGET})

    # compile host code
    set(HOST_TARGET ${TARGET_NAME}_host)
    add_executable(${HOST_TARGET} ${HOST_SOURCE})
    target_include_directories(${HOST_TARGET} PRIVATE ${WORKDIR})
    set_target_properties(${HOST_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${WORKDIR})
    target_link_libraries(${HOST_TARGET} ${SMI_LIBS})
    add_dependencies(${HOST_TARGET} ${HOST_GENERATED_TARGET})

    # generate emulation
    set(EMULATOR_TARGET ${TARGET_NAME}_emulator)
    add_custom_target(${EMULATOR_TARGET} )
    math(EXPR LOOP_END "${NUM_RANKS} - 1")
    foreach(SMI_EMULATION_RANK RANGE 0 ${LOOP_END} 1)
        #[[if(SMI_EMULATION_RANK EQUAL 0)
            set(KERNEL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}_rank0.cl)
        else()
            set(KERNEL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}_rank1.cl)
        endif()

        set(EMULATOR_TARGET_RANK ${EMULATOR_TARGET}_${SMI_EMULATION_RANK})
        add_custom_target(${EMULATOR_TARGET_RANK}
            COMMAND ${IntelFPGAOpenCL_AOC} ${AOC_COMMAND} ${KERNEL_FILE} -march=emulator
            -DSMI_EMULATION_RANK=${SMI_EMULATION_RANK}
            -emulator-channel-depth-model=strict
            -o ${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}.aocx)
        add_dependencies(${EMULATOR_TARGET} ${EMULATOR_TARGET_RANK})
        add_dependencies(${EMULATOR_TARGET_RANK} ${ROUTING_TARGET})#]]
    endforeach()
endfunction()

#################################
#TESTING
################################
enable_testing()
function(add_smi_test_executable TEST_PATH)
    get_filename_component(TEST_NAME ${TEST_PATH} NAME_WE)
    get_filename_component(DIRECTORY_NAME ${TEST_PATH} DIRECTORY)
    add_executable(${TEST_NAME}.exe ${DIRECTORY_NAME}/${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME}.exe ${SMI_LIBS} ${GTEST_LIBS})
endfunction()

##Target for building test program
# NUM_RANKS is used for generating the right emulations binaries
# MPMD used for the benchmarks described as MPMD programs
function(add_smi_test KERNEL_PATH NUM_RANKS MPMD)
    get_filename_component(KERNEL_NAME ${KERNEL_PATH} NAME_WE)
    get_filename_component(DIRECTORY_NAME ${KERNEL_PATH} NAME_WE)
    #code-gen

    add_custom_target(generate_test_${KERNEL_NAME}_routing
        COMMAND ${PYTHON_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/routing/main.py build ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY_NAME}/${KERNEL_NAME}.json  ${TEST_ROUTING_FILE}
        ${KERNEL_NAME}_routing)
    #add_dependencies(generate_${KERNEL_NAME} generate_${KERNEL_NAME}_routing)
    set(AOC_COMMAND
        -I${CMAKE_SOURCE_DIR}/include
        -I${CMAKE_SOURCE_DIR}/hlslib/include
        -I${CMAKE_CURRENT_SOURCE_DIR}/include
        -I${CMAKE_BINARY_DIR}
        -I${CMAKE_CURRENT_BINARY_DIR}
        -I${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}_routing
        -fp-relaxed
        -no-interleaving=default
        -fmax=${SMI_FMAX}
        -board=${SMI_TARGET_BOARD}
        -emulator-channel-depth-model=strict
        )

    if(MPMD)
        # To emulate an MPI-like setup, we need to generate a separate emulation
        # kernel for each rank
        add_custom_target(build_test_${KERNEL_NAME}_emulator )
        math(EXPR LOOP_END "${NUM_RANKS} - 1")
        foreach(SMI_EMULATION_RANK RANGE 0 ${LOOP_END} 1)
            if(SMI_EMULATION_RANK EQUAL 0)
                set(KERNEL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY_NAME}/${KERNEL_NAME}_rank0.cl)
            else()
                set(KERNEL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY_NAME}/${KERNEL_NAME}_rank1.cl)
            endif()
            add_custom_target(build_test_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}
                COMMAND ${IntelFPGAOpenCL_AOC}
                ${AOC_COMMAND} ${KERNEL_FILE} -march=emulator
                -DSMI_EMULATION_RANK=${SMI_EMULATION_RANK}
                -emulator-channel-depth-model=strict
                -o ${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}.aocx)
            add_dependencies(build_test_${KERNEL_NAME}_emulator
                build_test_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK})
            add_dependencies(build_test_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}
                generate_test_${KERNEL_NAME}_routing)
        endforeach()
    else()
        # To emulate an MPI-like setup, we need to generate a separate emulation
        # kernel for each rank
        add_custom_target(build_test_${KERNEL_NAME}_emulator )
        math(EXPR LOOP_END "${NUM_RANKS} - 1")
        foreach(SMI_EMULATION_RANK RANGE 0 ${LOOP_END} 1)
            set(KERNEL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY_NAME}/${KERNEL_NAME}.cl)
            add_custom_target(build_test_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}
                COMMAND ${IntelFPGAOpenCL_AOC}
                ${AOC_COMMAND} ${KERNEL_FILE} -march=emulator
                -DSMI_EMULATION_RANK=${SMI_EMULATION_RANK}
                -emulator-channel-depth-model=strict
                -o ${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}.aocx)
            add_dependencies(build_test_${KERNEL_NAME}_emulator
                build_test_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK})
            add_dependencies(build_test_${KERNEL_NAME}_emulator_${SMI_EMULATION_RANK}
                generate_test_${KERNEL_NAME}_routing)
        endforeach()
    endif()
    add_dependencies(build_test_${KERNEL_NAME}_emulator generate_test_${KERNEL_NAME}_routing)
endfunction()

add_subdirectory(source-rewriter)
#add_subdirectory(examples)
add_subdirectory(microbenchmarks)
if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(test)
endif (ENABLE_TESTS)

#add_subdirectory(rodinia)

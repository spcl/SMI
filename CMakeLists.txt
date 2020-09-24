cmake_minimum_required(VERSION 3.0)
project(smi)

# Configuration
set(SMI_TARGET_BOARD "p520_max_sg280l" CACHE STRING "aoc target board.")

set(SMI_REWRITER "${CMAKE_BINARY_DIR}/source-rewriter/rewriter")

set(SMI_FMAX "480" CACHE STRING "Target Fmax when compiling for hardware.")
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

# C++14 required
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


include_directories(SYSTEM ${MPI_CXX_INCLUDE_PATH})
set(SMI_LIBS ${CMAKE_THREAD_LIBS_INIT} ${IntelFPGAOpenCL_LIBRARIES} ${MPI_CXX_LIBRARIES})
set(GTEST_LIBS "-lgtest")

# Build flags for host executables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -DHLSLIB_INTEL ${MPI_CXX_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")


function(smi_target TARGET_NAME CONNECTION_FILE HOST_SOURCE KERNELS NUM_RANKS)
    set(SMI_SCRIPT ${CMAKE_SOURCE_DIR}/codegen/main.py)

    # parse optional arguments
    set(EXTRA_ARGS ${ARGN})
    set(OPT_CONSECUTIVE_READS 8)
    set(OPT_MAX_RANKS 8)
    set(OPT_P2P_RENDEZVOUS ON)

    list(LENGTH EXTRA_ARGS EXTRA_ARGS_COUNT)
    if(${EXTRA_ARGS_COUNT} GREATER 0)
        list(GET EXTRA_ARGS 0 OPT_CONSECUTIVE_READS)
    endif()
    if(${EXTRA_ARGS_COUNT} GREATER 1)
        list(GET EXTRA_ARGS 1 OPT_MAX_RANKS)
    endif()
    if(${EXTRA_ARGS_COUNT} GREATER 2)
        list(GET EXTRA_ARGS 2 OPT_P2P_RENDEZVOUS)
    endif()

    set(PROGRAM_METADATA)           # list of produced JSON metadata (one per program)
    set(KERNEL_TARGETS)             # list of targets (one per program)
    set(FPGA_SOURCES)               # list of transformed user device files (one per program)
    set(FPGA_GENERATED_SOURCES)     # list of generated device files (one per program)

    set(WORKDIR ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME})
    file(MAKE_DIRECTORY ${WORKDIR})

    set(AOC_COMMAND
        -I${CMAKE_SOURCE_DIR}/include
        -I${CMAKE_SOURCE_DIR}/hlslib/include
        -I${CMAKE_CURRENT_BINARY_DIR}
        -fp-relaxed
        -no-interleaving=default
        -board=${SMI_TARGET_BOARD}
    )

    # codegen and compile all programs
    foreach(KERNEL IN ITEMS ${KERNELS})
        get_filename_component(KERNEL_NAME ${KERNEL} NAME_WE)
        get_filename_component(KERNEL_NAME_EXT ${KERNEL} NAME)
        get_filename_component(KERNEL_SRC_DIR ${KERNEL} DIRECTORY)
        set(KERNEL_BIN_DIR ${WORKDIR}/${KERNEL_NAME})
        set(KERNEL_GENERATED_PATH ${KERNEL_BIN_DIR}/${KERNEL_NAME_EXT})
        set(SMI_GENERATED_PATH ${KERNEL_BIN_DIR}/smi_generated_device.cl)
        file(RELATIVE_PATH KERNEL_GENERATED_RELATIVE ${KERNEL_SRC_DIR} ${KERNEL})
        set(KERNEL_TARGET ${TARGET_NAME}_${KERNEL_NAME}_codegen_device)
        set(KERNEL_METADATA ${KERNEL_NAME}.json)
        list(APPEND PROGRAM_METADATA ${WORKDIR}/${KERNEL_METADATA})
        list(APPEND KERNEL_TARGETS ${KERNEL_TARGET})
        list(APPEND FPGA_SOURCES ${KERNEL_GENERATED_PATH})
        list(APPEND FPGA_GENERATED_SOURCES ${SMI_GENERATED_PATH})

        add_custom_target(${KERNEL_TARGET}
            COMMAND python3
                ${SMI_SCRIPT} codegen-device
                --include '${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}'
                --consecutive-read-limit '${OPT_CONSECUTIVE_READS}'
                --max-ranks '${OPT_MAX_RANKS}'
                --p2p-rendezvous '${OPT_P2P_RENDEZVOUS}'
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
            COMMAND ${IntelFPGAOpenCL_AOC} ${AOC_COMMAND} ${FPGA_SRC_FILES}
            WORKING_DIRECTORY ${KERNEL_BIN_DIR}
        )
        add_dependencies(${FPGA_BUILD_TARGET} ${KERNEL_TARGET})
    endforeach()

    # generate routing
    set(ROUTING_TARGET ${TARGET_NAME}_${KERNEL_NAME}_routing)
    add_custom_target(${ROUTING_TARGET}
            COMMAND python3
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
    set(SMI_HOST_GENERATED_PATH ${WORKDIR}/smi_generated_host.c)
    add_custom_target(${HOST_GENERATED_TARGET}
            COMMAND python3
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
    list(LENGTH KERNELS KERNELS_LENGTH)
    add_custom_target(${EMULATOR_TARGET} )

    math(EXPR LOOP_END "${NUM_RANKS} - 1")
    foreach(SMI_EMULATION_RANK RANGE 0 ${LOOP_END} 1)
        if(${KERNELS_LENGTH} EQUAL 1)
            set(KERNEL_INDEX 0)
        else()
            if(SMI_EMULATION_RANK EQUAL 0)
                set(KERNEL_INDEX 0)
            else()
                set(KERNEL_INDEX 1)
            endif()
        endif()

        list(GET FPGA_SOURCES ${KERNEL_INDEX} USER_DEVICE_SRC)
        list(GET FPGA_GENERATED_SOURCES ${KERNEL_INDEX} SMI_DEVICE_SRC)
        set(FPGA_SRC_FILES "${SMI_DEVICE_SRC};${USER_DEVICE_SRC}")

        set(EMULATION_WORKDIR ${WORKDIR}/emulator_${SMI_EMULATION_RANK})
        file(MAKE_DIRECTORY ${EMULATION_WORKDIR})
        set(EMULATOR_TARGET_RANK ${EMULATOR_TARGET}_${SMI_EMULATION_RANK})
        add_custom_target(${EMULATOR_TARGET_RANK}
            COMMAND ${IntelFPGAOpenCL_AOC} ${AOC_COMMAND} ${FPGA_SRC_FILES} -march=emulator
            -DSMI_EMULATION_RANK=${SMI_EMULATION_RANK}
            -emulator-channel-depth-model=strict
            WORKING_DIRECTORY ${EMULATION_WORKDIR}
        )
        add_dependencies(${EMULATOR_TARGET} ${EMULATOR_TARGET_RANK})
        add_dependencies(${EMULATOR_TARGET_RANK} ${ROUTING_TARGET})
    endforeach()
endfunction()



#used for non SMI target
function(fpga_target TARGET_NAME HOST_SOURCE KERNEL GENERATE_KERNEL)

    set(FPGA_SOURCES)               # list of transformed user device files (one per program)
    set(FPGA_GENERATED_SOURCES)     # list of generated device files (one per program)

    set(WORKDIR ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME})
    file(MAKE_DIRECTORY ${WORKDIR})
    get_filename_component(KERNEL_NAME ${KERNEL} NAME_WE)
    get_filename_component(KERNEL_NAME_EXT ${KERNEL} NAME)
    get_filename_component(KERNEL_SRC_DIR ${KERNEL} DIRECTORY)

    if(GENERATE_KERNEL)
           add_custom_target(generate_${KERNEL_NAME}
               COMMAND ${PYTHON_EXECUTABLE}
               ${CMAKE_CURRENT_SOURCE_DIR}/kernels/${KERNEL_NAME}.py
               ${SMI_STENCIL_NUM_PROCS_X} ${SMI_STENCIL_NUM_PROCS_Y}
               ${CMAKE_CURRENT_SOURCE_DIR}/kernels ${CMAKE_CURRENT_BINARY_DIR})
           set(KERNEL ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}.cl)
    endif()


    set(AOC_COMMAND
        -I${CMAKE_SOURCE_DIR}/include
        -I${CMAKE_SOURCE_DIR}/hlslib/include
        -I${CMAKE_CURRENT_BINARY_DIR}
        -fp-relaxed
        -no-interleaving=default
        -fmax=${SMI_FMAX}
        -board=${SMI_TARGET_BOARD}
    )


    set(KERNEL_BIN_DIR ${WORKDIR}/${KERNEL_NAME})

    # generate report
    set(FPGA_REPORT_TARGET ${TARGET_NAME}_${KERNEL_NAME}_aoc_report)
    add_custom_target(${FPGA_REPORT_TARGET}
            COMMAND ${IntelFPGAOpenCL_AOC} ${AOC_COMMAND} ${KERNEL} -rtl -report
            WORKING_DIRECTORY ${WORKDIR}
    )

    # build hardware
    set(FPGA_BUILD_TARGET ${TARGET_NAME}_${KERNEL_NAME}_aoc_build)
    add_custom_target(${FPGA_BUILD_TARGET}
        COMMAND ${IntelFPGAOpenCL_AOC} ${AOC_COMMAND} ${KERNEL}
        WORKING_DIRECTORY ${KERNEL_BIN_DIR}
    )
    if(USE_CODEGEN)
        add_dependencies(${FPGA_BUILD_TARGET} generate_${KERNEL_NAME})
    endif()

    # compile host code
    set(HOST_TARGET ${TARGET_NAME}_host)
    add_executable(${HOST_TARGET} ${HOST_SOURCE})
    target_include_directories(${HOST_TARGET} PRIVATE ${WORKDIR})
    set_target_properties(${HOST_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${WORKDIR})
    target_link_libraries(${HOST_TARGET} ${SMI_LIBS})


    # generate emulation
    set(EMULATOR_TARGET ${TARGET_NAME}_emulator)

    set(EMULATION_WORKDIR ${WORKDIR}/emulator)
    file(MAKE_DIRECTORY ${EMULATION_WORKDIR})
    add_custom_target(${EMULATOR_TARGET}
        COMMAND ${IntelFPGAOpenCL_AOC} ${AOC_COMMAND} ${KERNEL} -march=emulator
        -emulator-channel-depth-model=strict 
        WORKING_DIRECTORY ${EMULATION_WORKDIR}
    )
    if(USE_CODEGEN)
        add_dependencies(${EMULATOR_TARGET} generate_${KERNEL_NAME})
    endif()
endfunction()



add_subdirectory(source-rewriter)
add_subdirectory(examples)
add_subdirectory(microbenchmarks)
if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(test)
endif (ENABLE_TESTS)


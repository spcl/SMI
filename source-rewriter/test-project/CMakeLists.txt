project(weave-test)

function(smi_weave python_script connections rewriter target out_src_host)
    get_target_property(DEVICE_SOURCES ${target} SOURCES)

    set(WEAVED)
    set(HOST_GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/_host.c)
    set(DEVICE_GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/_device.c)
    set(${out_src_host} ${HOST_GENERATED_SRC} PARENT_SCOPE)

    foreach(file IN ITEMS ${DEVICE_SOURCES})
        set(SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${file})

        get_filename_component(FILE_NAME ${SRC_PATH} NAME_WE)
        get_filename_component(FILE_DIR ${SRC_PATH} DIRECTORY)
        set(DEST_PATH ${CMAKE_CURRENT_BINARY_DIR}/${file})
        get_filename_component(DEST_DIR ${DEST_PATH} DIRECTORY)
        message("Transforming ${SRC_PATH} into ${DEST_PATH}")
        file(COPY ${SRC_PATH} DESTINATION ${DEST_DIR})

        target_include_directories(${target} PRIVATE ${FILE_DIR})

        list(APPEND WEAVED ${DEST_PATH})
    endforeach()

    get_target_property(DEVICE_INCLUDE_DIRS ${target} INCLUDE_DIRECTORIES)
    add_custom_command(
            OUTPUT ${DEVICE_GENERATED_SRC}
            COMMAND python ${python_script} build --include '${DEVICE_INCLUDE_DIRS}' ${connections} ${rewriter} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${HOST_GENERATED_SRC} ${DEVICE_GENERATED_SRC} ${DEVICE_SOURCES}
            DEPENDS ${DEVICE_SOURCES}
    )

    file(WRITE ${HOST_GENERATED_SRC})
    file(WRITE ${DEVICE_GENERATED_SRC})
    target_sources(${target} PRIVATE ${WEAVED} ${DEVICE_GENERATED_SRC})
endfunction()

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR}/)
set(PYTHON_SCRIPT ${PROJECT_ROOT}/routing/main.py)
set(REWRITER ${CMAKE_CURRENT_BINARY_DIR}/../rewriter)
set(CONNECTIONS ${PROJECT_ROOT}/routing/fpga-06.routing)

set(SOURCES_DEVICE device.cl nested/device-nested.c)
add_executable(weave_device ${SOURCES_DEVICE})
target_include_directories(weave_device PRIVATE ${PROJECT_ROOT}/include)
smi_weave(${PYTHON_SCRIPT} ${CONNECTIONS} ${REWRITER} weave_device "SOURCES_HOST_WEAVED")

add_executable(weave_host host.c ${SOURCES_HOST_WEAVED})

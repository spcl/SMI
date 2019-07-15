find_package(Clang)

set(SOURCES
        src/main.cpp
        src/action.cpp
        src/rewrite.cpp
        src/utils.cpp
        src/ops.cpp
)

add_executable(rewriter ${SOURCES})
target_link_libraries(rewriter clangRewrite clangAST clangTooling)
target_include_directories(rewriter PRIVATE ${CLANG_INCLUDE_DIRS})

#add_subdirectory(test-project)

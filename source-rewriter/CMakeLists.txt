find_package(Clang)

set(SOURCES
        src/main.cpp
        src/action.cpp
        src/rewrite.cpp
        src/utils.cpp
        src/ops/ops.cpp
        src/ops/push.cpp
        src/ops/utils.cpp
        src/ops/pop.cpp
        src/ops/broadcast.cpp
        src/ops/scatter.cpp
        src/ops/gather.cpp
        src/ops/reduce.cpp
)

add_executable(rewriter ${SOURCES})
target_link_libraries(rewriter clangRewrite clangAST clangTooling clangBasic clangLex clangFrontend LLVM)
target_include_directories(rewriter PRIVATE ${CLANG_INCLUDE_DIRS})

#pragma once

#include <clang/AST/Decl.h>

bool isKernelFunction(clang::FunctionDecl* decl);

template <typename T>
inline std::vector<clang::Stmt*> gatherStatements(T* decl)
{
    std::vector<clang::Stmt*> children;
    for (auto& child: decl->children())
    {
        children.push_back(child);
    }
    return children;
}

#include "utils.h"

using namespace clang;

bool isKernelFunction(FunctionDecl* decl)
{
    if (!decl) return false;
    if (!decl->hasBody()) return false;

    for (auto& attr: decl->attrs())
    {
        if (std::string(attr->getSpelling()) == "__kernel") return true;
    }

    return false;
}

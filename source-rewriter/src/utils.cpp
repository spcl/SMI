#include "utils.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Attr.h>
#include <unordered_set>

using namespace clang;

bool isKernelFunction(FunctionDecl* decl)
{
    if (!decl) return false;
    if (!decl->hasBody()) return false;

    std::unordered_set<std::string> validAttrs = {
            "kernel",
            "__kernel"
    };

    for (auto& attr: decl->attrs())
    {
        auto spelling = std::string(attr->getSpelling());
        if (validAttrs.find(spelling) != validAttrs.end())
        {
            return true;
        }
    }

    return false;
}

#include "action.h"

#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

using namespace clang::tooling;

static llvm::cl::OptionCategory Opt("SMI kernel rewriter");

int main(int argc, const char** argv)
{
    CommonOptionsParser op(argc, argv, Opt);
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());

    return Tool.run(clang::tooling::newFrontendActionFactory<SpecializeCallsAction>().get());
}

// TODO: matchers
// TODO: run action manually

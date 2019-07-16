#include "action.h"

#include <fstream>
#include <iostream>

#include <clang/Frontend/CompilerInstance.h>

using namespace clang;
using namespace llvm;

void SpecializeCallsAction::EndSourceFileAction()
{
    this->rewriter.overwriteChangedFiles();
}

std::unique_ptr<ASTConsumer> SpecializeCallsAction::CreateASTConsumer(
        CompilerInstance& CI,
        StringRef file)
{
    this->rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
//    CI.getDiagnostics().setClient(new IgnoringDiagConsumer());
    return std::make_unique<SpecializeCallsConsumer>(this->rewriter);
}

bool SpecializeCallsConsumer::HandleTopLevelDecl(DeclGroupRef DR)
{
    for (auto& decl : DR)
    {
        this->visitor.TraverseDecl(decl);
    }
    return true;
}

#include "action.h"

#include <fstream>
#include <iostream>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/PreprocessorOptions.h>

using namespace clang;
using namespace llvm;

bool SpecializeCallsConsumer::HandleTopLevelDecl(DeclGroupRef group)
{
    for (auto& decl: group)
    {
        this->visitor.TraverseDecl(decl);
    }
    return true;
}

bool SpecializeCallsAction::PrepareToExecuteAction(CompilerInstance& compiler)
{
    compiler.getPreprocessorOpts().addMacroDef("SMI_REWRITER");
    compiler.getDiagnostics().setErrorLimit(9999);
    return true;
}
std::unique_ptr<ASTConsumer> SpecializeCallsAction::CreateASTConsumer(
        CompilerInstance& compiler,
        StringRef file)
{
    this->rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
    return std::make_unique<SpecializeCallsConsumer>(this->rewriter);
}
void SpecializeCallsAction::EndSourceFileAction()
{
    this->rewriter.overwriteChangedFiles();
}

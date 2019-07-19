#pragma once

#include "rewrite.h"

#include <clang/Frontend/FrontendAction.h>
#include <clang/Rewrite/Core/Rewriter.h>

class SpecializeCallsConsumer: public clang::ASTConsumer
{
public:
    explicit SpecializeCallsConsumer(clang::Rewriter& rewriter)
    : visitor(rewriter)
    {

    }

    bool HandleTopLevelDecl(clang::DeclGroupRef group) override;

private:
    RewriteKernelsVisitor visitor;
};

class SpecializeCallsAction: public clang::ASTFrontendAction
{
public:
    bool PrepareToExecuteAction(clang::CompilerInstance& compiler) override;

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
            clang::CompilerInstance& compiler,
            llvm::StringRef file) override;
    void EndSourceFileAction() override;

private:
    clang::Rewriter rewriter;
};

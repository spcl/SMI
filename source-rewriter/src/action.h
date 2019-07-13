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

    bool HandleTopLevelDecl(clang::DeclGroupRef DR) override;

private:
    RewriteKernelsVisitor visitor;
};

class SpecializeCallsAction: public clang::ASTFrontendAction
{
public:
    void EndSourceFileAction() override;

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
            clang::CompilerInstance& CI,
            llvm::StringRef file) override;

private:
    clang::Rewriter rewriter;
};

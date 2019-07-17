#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>

class RewriteKernelsVisitor: public clang::RecursiveASTVisitor<RewriteKernelsVisitor>
{
public:
    explicit RewriteKernelsVisitor(clang::Rewriter& rewriter) : rewriter(rewriter) { }

    bool VisitFunctionDecl(clang::FunctionDecl *f);

private:
    clang::Rewriter& rewriter;
};

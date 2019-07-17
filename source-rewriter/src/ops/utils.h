#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <string>

#include "ops.h"

class FindIntegerLiteral: public clang::RecursiveASTVisitor<FindIntegerLiteral>
{
public:
    bool VisitIntegerLiteral(clang::IntegerLiteral* literal);
    bool VisitDeclRefExpr(clang::DeclRefExpr* expr);

    size_t getValue() const;

private:
    void setValue(size_t value);

    bool valueFound = false;
    size_t value;
};

std::string formatDataType(DataType dataType);

std::string renamePortDataType(const std::string& callName, const OperationMetadata& metadata);

size_t extractIntArg(clang::CallExpr* expr, int argumentIndex);
size_t extractBufferSize(clang::CallExpr* expr, int argumentIndex);
DataType extractDataType(clang::CallExpr* expr, int argumentIndex);
clang::CallExpr* extractChannelDecl(clang::CallExpr* expr);

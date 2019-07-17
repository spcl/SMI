#pragma once

#include "ops.h"

class BroadcastExtractor: public OperationExtractor
{
public:
    OperationMetadata GetOperationMetadata(clang::CallExpr* callExpr) override;
    std::string CreateDeclaration(const std::string& callName, const OperationMetadata& metadata) override;
    std::vector<std::string> GetFunctionNames() override;
};

class BroadcastChannelExtractor: public ChannelExtractor
{
public:
    OperationMetadata GetOperationMetadata(clang::CallExpr* callExpr) override;
    std::string CreateDeclaration(const std::string& callName, const OperationMetadata& metadata) override;
    std::string GetChannelFunctionName() override;
};

#include <utility>

#include <utility>

#pragma once
#include <ostream>
#include <cstddef>
#include <vector>
#include <string>

#include <clang/AST/AST.h>

class OperationMetadata
{
public:
    OperationMetadata(std::string operation, size_t port, std::vector<std::string> args = {})
    : operation(std::move(operation)), port(port), args(std::move(args))
    {

    }

    void output(std::ostream& os) const
    {
        os << this->operation << " " << this->port;
        for (auto& arg : this->args)
        {
            os << " " << arg;
        }
        os << std::endl;
    }

    std::string operation;
    size_t port;
    std::vector<std::string> args;
};

class OperationExtractor
{
public:
    virtual ~OperationExtractor() = default;

    virtual OperationMetadata GetOperationMetadata(clang::VarDecl* channelDecl) = 0;
    virtual std::string RenameCall(std::string callName, const OperationMetadata& metadata) = 0;
    virtual std::string CreateDeclaration(std::string callName, const OperationMetadata& metadata) = 0;
};

class PushExtractor: public OperationExtractor
{
public:
    OperationMetadata GetOperationMetadata(clang::VarDecl* channelDecl) override;
    std::string RenameCall(std::string callName, const OperationMetadata& metadata) override;
    std::string CreateDeclaration(std::string callName, const OperationMetadata& metadata) override;
};

class PopExtractor: public OperationExtractor
{
public:
    OperationMetadata GetOperationMetadata(clang::VarDecl* channelDecl) override;
    std::string RenameCall(std::string callName, const OperationMetadata& metadata) override;
    std::string CreateDeclaration(std::string callName, const OperationMetadata& metadata) override;
};

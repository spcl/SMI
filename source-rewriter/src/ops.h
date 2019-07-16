#include <utility>

#include <utility>

#pragma once
#include <ostream>
#include <cstddef>
#include <vector>
#include <string>

#include <clang/AST/AST.h>

enum class DataType {
    Char,
    Short,
    Int,
    Float,
    Double
};

class OperationMetadata
{
public:
    OperationMetadata(std::string operation, size_t port, DataType dataType = DataType::Int,
            std::vector<std::string> args = {})
    : operation(std::move(operation)), port(port), dataType(dataType), args(std::move(args))
    {

    }

    void output(std::ostream& os) const;

    std::string operation;
    size_t port;
    DataType dataType;
    std::vector<std::string> args;
};

class OperationExtractor
{
public:
    virtual ~OperationExtractor() = default;

    /**
     * Extract metadata from a channel declaration.
     * The metadata should contain the name of the operation, the used logical port and potentially other arguments
     * like data type or reduce operation.
     */
    virtual OperationMetadata GetOperationMetadata(clang::VarDecl* channelDecl) = 0;

    /**
     * Rename a function call given the extracted metadata.
     * For example for callName="SMI_Push" and Metadata with port 0, it should return "SMI_Push_0".
     */
    virtual std::string RenameCall(std::string callName, const OperationMetadata& metadata) = 0;

    /**
     * Forward declare a renamed function call.
     * For example if RenameCall returned "SMI_Push_0" for a given metadata, this function should return
     * "void SMI_Push_0(SMI_Channel *chan, void* data);" for the same metadata.
     */
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

class BroadcastExtractor: public OperationExtractor
{
public:
    OperationMetadata GetOperationMetadata(clang::VarDecl* channelDecl) override;
    std::string RenameCall(std::string callName, const OperationMetadata& metadata) override;
    std::string CreateDeclaration(std::string callName, const OperationMetadata& metadata) override;
};

class ScatterExtractor: public OperationExtractor
{
public:
    OperationMetadata GetOperationMetadata(clang::VarDecl* channelDecl) override;
    std::string RenameCall(std::string callName, const OperationMetadata& metadata) override;
    std::string CreateDeclaration(std::string callName, const OperationMetadata& metadata) override;
};

class GatherExtractor: public OperationExtractor
{
public:
    OperationMetadata GetOperationMetadata(clang::VarDecl* channelDecl) override;
    std::string RenameCall(std::string callName, const OperationMetadata& metadata) override;
    std::string CreateDeclaration(std::string callName, const OperationMetadata& metadata) override;
};

class ReduceExtractor: public OperationExtractor
{
public:
    OperationMetadata GetOperationMetadata(clang::VarDecl* channelDecl) override;
    std::string RenameCall(std::string callName, const OperationMetadata& metadata) override;
    std::string CreateDeclaration(std::string callName, const OperationMetadata& metadata) override;
};

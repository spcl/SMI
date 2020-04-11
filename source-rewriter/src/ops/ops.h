#pragma once

#include <ostream>
#include <unordered_map>
#include <string>
#include <sstream>

#include <clang/AST/Expr.h>
#include <clang/Rewrite/Core/Rewriter.h>

enum class DataType {
    Char,
    Short,
    Int,
    Float,
    Double,
    Float2,
    Float4,
    Double2
};

class OperationMetadata
{
public:
    OperationMetadata(std::string operation,
            size_t port,
            DataType dataType = DataType::Int,
            int bufferSize = -1,
            std::unordered_map<std::string, std::string> args = {})
    : operation(std::move(operation)), port(port), dataType(dataType), bufferSize(bufferSize), args(std::move(args))
    {

    }

    bool isBufferSizeSet() const
    {
        return this->bufferSize != -1;
    }

    std::string operation;
    size_t port;
    DataType dataType;
    int bufferSize;
    std::unordered_map<std::string, std::string> args;
};

class OperationExtractor
{
public:
    virtual ~OperationExtractor() = default;

    /**
     * Extract metadata from a call expression.
     * The metadata should contain the name of the operation, the used logical port and potentially other arguments
     * like data type or reduce operation.
     */
    virtual OperationMetadata GetOperationMetadata(clang::CallExpr* callExpr) = 0;

    /**
     * Rename a function call given the extracted metadata.
     * For example for callName="SMI_Push" and Metadata with port 0, it should return "SMI_Push_0".
     */
    virtual std::string RenameCall(const std::string& callName, const OperationMetadata& metadata);

    /**
     * Forward declare a renamed function call.
     * For example if RenameCall returned "SMI_Push_0" for a given metadata, this function should return
     * "void SMI_Push_0(SMI_Channel *chan, void* data);" for the same metadata.
     */
    virtual std::string CreateDeclaration(const std::string& callName, const OperationMetadata& metadata) = 0;

    /**
     * Outputs the serialized metadata to the given stream.
     */
    virtual void OutputMetadata(const OperationMetadata& metadata, std::ostream& os)
    {

    }

    /**
     * Returns the function names that should invoke this extractor.
     */
    virtual std::vector<std::string> GetFunctionNames() = 0;

    /**
     * Rewrite the function call if necessary.
     */
    virtual OperationMetadata ModifyCall(clang::Rewriter& rewriter, clang::CallExpr& callExpr, const std::string& callName);
};

class ChannelExtractor: public OperationExtractor
{
public:
    std::string RenameCall(const std::string& callName, const OperationMetadata& metadata) override;

    /**
     * Outputs the serialized metadata to the given stream.
     */
    void OutputMetadata(const OperationMetadata& metadata, std::ostream& os) override;
    std::vector<std::string> GetFunctionNames() final;

    OperationMetadata ModifyCall(clang::Rewriter& rewriter, clang::CallExpr& callExpr, const std::string& callName) override;

    virtual std::string GetChannelFunctionName() = 0;

protected:
    std::string CreateChannelDeclaration(
            const std::string& callName,
            const OperationMetadata& metadata,
            const std::string& returnType,
            const std::string& parameters);
};

#include "ops.h"

#include <clang/AST/RecursiveASTVisitor.h>

using namespace clang;

static std::string renamePort(const std::string& callName, const OperationMetadata& metadata)
{
    return callName + "_" + std::to_string(metadata.port);
}
static std::string createChannelDataDeclaration(const std::string& name)
{
    return "void " + name + "(SMI_Channel* chan, void* data);";
}

class FindIntegerLiteral: public clang::RecursiveASTVisitor<FindIntegerLiteral>
{
public:
    bool VisitIntegerLiteral(IntegerLiteral* literal)
    {
        assert(!this->literal);
        this->literal = literal;

        return false;
    }

    IntegerLiteral* getLiteral() const
    {
        assert(this->literal);
        return this->literal;
    }

private:
    IntegerLiteral* literal = nullptr;
};

static size_t extractIntArg(VarDecl* channelDecl, int argumentIndex)
{
    auto init = dyn_cast<CallExpr>(channelDecl->getInit());
    assert(init);
    auto arg = init->getArgs()[argumentIndex];

    FindIntegerLiteral visitor;
    visitor.TraverseStmt(arg);
    return visitor.getLiteral()->getValue().getZExtValue();
}

OperationMetadata PushExtractor::GetOperationMetadata(VarDecl* channelDecl)
{
    return OperationMetadata("push", extractIntArg(channelDecl, 3));
}
std::string PushExtractor::RenameCall(std::string callName, const OperationMetadata& metadata)
{
    return renamePort(callName, metadata);
}

std::string PushExtractor::CreateDeclaration(std::string callName, const OperationMetadata& metadata)
{
    std::string args = "(SMI_Channel* chan, void* data";
    if (callName == "SMI_Push_flush")
    {
        args += ", int immediate";
    }

    return "void " + this->RenameCall(callName, metadata) + args + ");";
}

OperationMetadata PopExtractor::GetOperationMetadata(clang::VarDecl* channelDecl)
{
    return OperationMetadata("pop", extractIntArg(channelDecl, 3));
}
std::string PopExtractor::RenameCall(std::string callName, const OperationMetadata& metadata)
{
    return renamePort(callName, metadata);
}

std::string PopExtractor::CreateDeclaration(std::string callName, const OperationMetadata& metadata)
{
    return createChannelDataDeclaration(this->RenameCall(callName, metadata));
}

OperationMetadata BroadcastExtractor::GetOperationMetadata(clang::VarDecl* channelDecl)
{
    return OperationMetadata("broadcast", extractIntArg(channelDecl, 2));
}

std::string BroadcastExtractor::RenameCall(std::string callName, const OperationMetadata& metadata)
{
    return renamePort(callName, metadata);
}

std::string BroadcastExtractor::CreateDeclaration(std::string callName, const OperationMetadata& metadata)
{
    return createChannelDataDeclaration(this->RenameCall(callName, metadata));
}

#include "ops.h"

#include <clang/AST/RecursiveASTVisitor.h>

using namespace clang;

static std::string renameP2P(std::string& callName, const OperationMetadata& metadata)
{
    return callName + "_" + std::to_string(metadata.port);
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
    return renameP2P(callName, metadata);
}

std::string PushExtractor::CreateDeclaration(std::string callName, const OperationMetadata& metadata)
{
    return "void " + this->RenameCall(callName, metadata) + "(SMI_Channel *chan, void* data);";
}

OperationMetadata PopExtractor::GetOperationMetadata(clang::VarDecl* channelDecl)
{
    return OperationMetadata("pop", extractIntArg(channelDecl, 3));
}
std::string PopExtractor::RenameCall(std::string callName, const OperationMetadata& metadata)
{
    return renameP2P(callName, metadata);
}

std::string PopExtractor::CreateDeclaration(std::string callName, const OperationMetadata& metadata)
{
    return "void " + this->RenameCall(callName, metadata) + "SMI_Channel *chan, void* data);";
}

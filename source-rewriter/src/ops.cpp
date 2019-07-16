#include "ops.h"

#include <clang/AST/RecursiveASTVisitor.h>

using namespace clang;

static std::string formatDataType(DataType dataType)
{
    switch (dataType)
    {
        case DataType::Char: return "char";
        case DataType::Short: return "short";
        case DataType::Int: return "int";
        case DataType::Float: return "float";
        case DataType::Double: return "double";
    }

    assert(false);
    return "";
}

static std::string renamePortDataType(const std::string& callName, const OperationMetadata& metadata)
{
    auto call = callName + "_" + std::to_string(metadata.port);
    return call + "_" + formatDataType(metadata.dataType);
}

class FindIntegerLiteral: public clang::RecursiveASTVisitor<FindIntegerLiteral>
{
public:
    bool VisitIntegerLiteral(IntegerLiteral* literal)
    {
        this->setValue(literal->getValue().getZExtValue());
        return false;
    }
    bool VisitDeclRefExpr(DeclRefExpr* expr)
    {
        auto decl = expr->getDecl();
        if (auto enumeration = dyn_cast<EnumConstantDecl>(decl))
        {
            this->setValue(enumeration->getInitVal().getZExtValue());
        }
        return false;
    }

    size_t getValue() const
    {
        assert(this->valueFound);
        return this->value;
    }

private:
    void setValue(size_t value)
    {
        assert(!this->valueFound);
        this->valueFound = true;
        this->value = value;
    }

    bool valueFound = false;
    size_t value;
};

static size_t extractIntArg(VarDecl* channelDecl, int argumentIndex)
{
    auto init = dyn_cast<CallExpr>(channelDecl->getInit());
    assert(init);
    auto arg = init->getArgs()[argumentIndex];

    FindIntegerLiteral visitor;
    visitor.TraverseStmt(arg);
    return visitor.getValue();
}
static DataType extractDataType(VarDecl* channelDecl, int argumentIndex)
{
    size_t arg = extractIntArg(channelDecl, argumentIndex);
    assert(arg >= 1 && arg <= 5);

    switch (arg)
    {
        case 1: return DataType::Int;
        case 2: return DataType::Float;
        case 3: return DataType::Double;
        case 4: return DataType::Char;
        case 5: return DataType::Short;
        default:
            assert(false);
    }
    return DataType::Int;
}

void OperationMetadata::output(std::ostream& os) const
{
    os << this->operation << " " << this->port << " ";
    os << formatDataType(this->dataType);

    for (auto& arg : this->args)
    {
        os << " " << arg;
    }
    os << std::endl;
}

// Push
OperationMetadata PushExtractor::GetOperationMetadata(VarDecl* channelDecl)
{
    return OperationMetadata("push", extractIntArg(channelDecl, 3));
}
std::string PushExtractor::RenameCall(std::string callName, const OperationMetadata& metadata)
{
    return renamePortDataType(callName, metadata);
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

// Pop
OperationMetadata PopExtractor::GetOperationMetadata(clang::VarDecl* channelDecl)
{
    return OperationMetadata("pop", extractIntArg(channelDecl, 3));
}
std::string PopExtractor::RenameCall(std::string callName, const OperationMetadata& metadata)
{
    return renamePortDataType(callName, metadata);
}
std::string PopExtractor::CreateDeclaration(std::string callName, const OperationMetadata& metadata)
{
    return "void " + this->RenameCall(callName, metadata) + "(SMI_Channel* chan, void* data);";
}

// Broadcast
OperationMetadata BroadcastExtractor::GetOperationMetadata(clang::VarDecl* channelDecl)
{
    return OperationMetadata("broadcast", extractIntArg(channelDecl, 2));
}
std::string BroadcastExtractor::RenameCall(std::string callName, const OperationMetadata& metadata)
{
    return renamePortDataType(callName, metadata);
}
std::string BroadcastExtractor::CreateDeclaration(std::string callName, const OperationMetadata& metadata)
{
    return "void " + this->RenameCall(callName, metadata) + "(SMI_BChannel* chan, void* data);";
}

// Scatter
OperationMetadata ScatterExtractor::GetOperationMetadata(clang::VarDecl* channelDecl)
{
    return OperationMetadata("scatter", extractIntArg(channelDecl, 3), extractDataType(channelDecl, 2));
}
std::string ScatterExtractor::RenameCall(std::string callName, const OperationMetadata& metadata)
{
    return renamePortDataType(callName, metadata);
}
std::string ScatterExtractor::CreateDeclaration(std::string callName, const OperationMetadata& metadata)
{
    return "void " + this->RenameCall(callName, metadata) + "(SMI_ScatterChannel* chan, void* data_snd, void* data_rcv);";
}

// Gather
OperationMetadata GatherExtractor::GetOperationMetadata(clang::VarDecl* channelDecl)
{
    return OperationMetadata("gather", extractIntArg(channelDecl, 3), extractDataType(channelDecl, 2));
}
std::string GatherExtractor::RenameCall(std::string callName, const OperationMetadata& metadata)
{
    return renamePortDataType(callName, metadata);
}
std::string GatherExtractor::CreateDeclaration(std::string callName, const OperationMetadata& metadata)
{
    return "void " + this->RenameCall(callName, metadata) + "(SMI_GatherChannel* chan, void* send_data, void* rcv_data);";
}

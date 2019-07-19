#include "utils.h"

using namespace clang;

bool FindIntegerLiteral::VisitIntegerLiteral(IntegerLiteral* literal)
{
    this->setValue(literal->getValue().getZExtValue());
    return false;
}
bool FindIntegerLiteral::VisitDeclRefExpr(DeclRefExpr* expr)
{
    auto decl = expr->getDecl();
    if (auto enumeration = dyn_cast<EnumConstantDecl>(decl))
    {
        this->setValue(enumeration->getInitVal().getZExtValue());
    }
    else if (auto varDecl = dyn_cast<VarDecl>(decl))
    {
        FindIntegerLiteral visitor;
        visitor.TraverseDecl(varDecl);
        if (visitor.valueFound)
        {
            this->value = visitor.value;
            this->valueFound = true;
        }
    }
    return false;
}
size_t FindIntegerLiteral::getValue() const
{
    assert(this->valueFound);
    return this->value;
}
void FindIntegerLiteral::setValue(size_t value)
{
    assert(!this->valueFound);
    this->valueFound = true;
    this->value = value;
}

size_t extractIntArg(CallExpr* expr, int argumentIndex)
{
    auto arg = expr->getArgs()[argumentIndex];

    FindIntegerLiteral visitor;
    visitor.TraverseStmt(arg);
    return visitor.getValue();
}

std::string formatDataType(DataType dataType)
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

std::string renamePortDataType(const std::string& callName, const OperationMetadata& metadata)
{
    auto call = callName + "_" + std::to_string(metadata.port);
    return call + "_" + formatDataType(metadata.dataType);
}

DataType extractDataType(CallExpr* expr, int argumentIndex)
{
    size_t arg = extractIntArg(expr, argumentIndex);
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

class FindVarDecl: public RecursiveASTVisitor<FindVarDecl>
{
public:
    bool VisitVarDecl(VarDecl* decl)
    {
        assert(!this->decl);
        this->decl = decl;

        return false;
    }
    bool VisitDeclRefExpr(DeclRefExpr* expr)
    {
        this->TraverseDecl(expr->getDecl());
        return true;
    }

    VarDecl* decl = nullptr;
};

CallExpr* extractChannelDecl(CallExpr* expr)
{
    assert(expr->getNumArgs() > 0);
    auto& arg = expr->getArgs()[0];
    FindVarDecl visitor;
    visitor.TraverseStmt(arg);
    assert(visitor.decl);
    auto channelDecl = visitor.decl;
    auto callExpr = dyn_cast<CallExpr>(channelDecl->getInit());
    assert(callExpr);
    return callExpr;
}

size_t extractBufferSize(CallExpr* callExpr, int argumentIndex)
{
    if (callExpr->getNumArgs() >= argumentIndex) return -1;
    return extractIntArg(callExpr, argumentIndex);
}

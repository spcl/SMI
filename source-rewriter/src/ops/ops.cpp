#include "ops.h"
#include "utils.h"

#include "../third-party/json.hpp"

#include <clang/Lex/Lexer.h>

using json = nlohmann::json;
using namespace clang;

std::string OperationExtractor::RenameCall(const std::string& callName, const OperationMetadata& metadata)
{
    return renamePortDataType(callName, metadata);
}

OperationMetadata OperationExtractor::ModifyCall(clang::Rewriter& rewriter, clang::CallExpr& callExpr, const std::string& callName)
{
    auto metadata = this->GetOperationMetadata(&callExpr);
    auto renamed = this->RenameCall(callName, metadata);
    rewriter.ReplaceText(callExpr.getBeginLoc(), renamed);
    return metadata;
}

void ChannelExtractor::OutputMetadata(const OperationMetadata& metadata, std::ostream& os)
{
    json obj;
    obj["type"] = metadata.operation;
    obj["port"] = metadata.port;
    obj["data_type"] = formatDataType(metadata.dataType);

    if (metadata.isBufferSizeSet())
    {
        obj["buffer_size"] = metadata.bufferSize;
    }
    else obj["buffer_size"] = nullptr;

    obj["args"] = metadata.args;

    os << obj.dump() << std::endl;
}

// https://stackoverflow.com/a/874160/1107768
static bool ends(const std::string& str, const std::string& end)
{
    if (str.length() >= end.length())
    {
        return (0 == str.compare(str.length() - end.length(), end.length(), end));
    }
    else return false;
}
static bool isExtendedChannelOpen(const std::string& callName)
{
    return ends(callName, "_ad");
}

std::string ChannelExtractor::CreateChannelDeclaration(const std::string& callName, const OperationMetadata& metadata,
        const std::string& returnType, const std::string& parameters)
{
    std::stringstream ss;
    ss << returnType << " " << this->RenameCall(callName, metadata) << "(" << parameters << ");";

    return ss.str();
}

std::string ChannelExtractor::RenameCall(const std::string& callName, const OperationMetadata& metadata)
{
    auto name = callName;
    if (isExtendedChannelOpen(name))
    {
        name.resize(name.size() - 3);
    }
    return OperationExtractor::RenameCall(name, metadata);
}

std::vector<std::string> ChannelExtractor::GetFunctionNames()
{
    auto name = this->GetChannelFunctionName();
    return {name, name + "_ad"};
}

OperationMetadata ChannelExtractor::ModifyCall(Rewriter& rewriter, CallExpr& callExpr, const std::string& callName)
{
    auto metadata = OperationExtractor::ModifyCall(rewriter, callExpr, callName);
    if (isExtendedChannelOpen(callName) && callExpr.getNumArgs() >= 2)
    {
        auto lastArg = callExpr.getArgs()[callExpr.getNumArgs() - 1];
        auto previousArg = callExpr.getArgs()[callExpr.getNumArgs() - 2];
        auto end0 = Lexer::getLocForEndOfToken(previousArg->getEndLoc(), 0, rewriter.getSourceMgr(), rewriter.getLangOpts());
        auto end1 = Lexer::getLocForEndOfToken(lastArg->getEndLoc(), 0, rewriter.getSourceMgr(), rewriter.getLangOpts());
        rewriter.RemoveText(CharSourceRange::getCharRange(end0, end1));
    }
    return metadata;
}

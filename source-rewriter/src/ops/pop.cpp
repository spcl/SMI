#include "pop.h"
#include "utils.h"

using namespace clang;

static OperationMetadata extractPop(CallExpr* channelDecl)
{
    return OperationMetadata("pop",
                             extractIntArg(channelDecl, 3),
                             extractDataType(channelDecl, 1),
                             extractBufferSize(channelDecl, 5)
    );
}

OperationMetadata PopExtractor::GetOperationMetadata(CallExpr* callExpr)
{
    return extractPop(extractChannelDecl(callExpr));
}
std::string PopExtractor::CreateDeclaration(const std::string& callName, const OperationMetadata& metadata)
{
    return "void " + this->RenameCall(callName, metadata) + "(SMI_Channel* chan, void* data);";
}
std::vector<std::string> PopExtractor::GetFunctionNames()
{
    return {"SMI_Pop"};
}

OperationMetadata PopChannelExtractor::GetOperationMetadata(CallExpr* callExpr)
{
    return extractPop(callExpr);
}
std::string PopChannelExtractor::CreateDeclaration(const std::string& callName, const OperationMetadata& metadata)
{
    return this->CreateChannelDeclaration(callName, metadata, "SMI_Channel", "int count, SMI_Datatype data_type, int source, int port, SMI_Comm comm");
}
std::string PopChannelExtractor::GetChannelFunctionName()
{
    return "SMI_Open_receive_channel";
}

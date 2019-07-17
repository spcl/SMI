#include "broadcast.h"
#include "utils.h"

using namespace clang;

static OperationMetadata extractBroadcast(CallExpr* channelDecl)
{
    return OperationMetadata("broadcast",
                             extractIntArg(channelDecl, 2),
                             extractDataType(channelDecl, 1),
                             extractBufferSize(channelDecl, 5)
    );
}

OperationMetadata BroadcastExtractor::GetOperationMetadata(CallExpr* callExpr)
{
    return extractBroadcast(extractChannelDecl(callExpr));
}
std::string BroadcastExtractor::CreateDeclaration(const std::string& callName, const OperationMetadata& metadata)
{
    return "void " + this->RenameCall(callName, metadata) + "(SMI_BChannel* chan, void* data);";
}
std::vector<std::string> BroadcastExtractor::GetFunctionNames()
{
    return {"SMI_Bcast"};
}

OperationMetadata BroadcastChannelExtractor::GetOperationMetadata(CallExpr* callExpr)
{
    return extractBroadcast(callExpr);
}
std::string BroadcastChannelExtractor::CreateDeclaration(const std::string& callName, const OperationMetadata& metadata)
{
    return this->CreateChannelDeclaration(callName, metadata, "SMI_BChannel", "int count, SMI_Datatype data_type, int port, int root, SMI_Comm comm");
}
std::string BroadcastChannelExtractor::GetChannelFunctionName()
{
    return "SMI_Open_bcast_channel";
}

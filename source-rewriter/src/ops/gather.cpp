#include "gather.h"
#include "utils.h"

using namespace clang;

static OperationMetadata extractGather(CallExpr* channelDecl)
{
    return OperationMetadata("gather",
                             extractIntArg(channelDecl, 3),
                             extractDataType(channelDecl, 2),
                             extractBufferSize(channelDecl, 6)
    );
}

OperationMetadata GatherExtractor::GetOperationMetadata(CallExpr* callExpr)
{
    return extractGather(extractChannelDecl(callExpr));
}
std::string GatherExtractor::CreateDeclaration(const std::string& callName, const OperationMetadata& metadata)
{
    return "void " + this->RenameCall(callName, metadata) + "(SMI_GatherChannel* chan, void* send_data, void* rcv_data);";
}
std::vector<std::string> GatherExtractor::GetFunctionNames()
{
    return {"SMI_Gather"};
}

OperationMetadata GatherChannelExtractor::GetOperationMetadata(CallExpr* callExpr)
{
    return extractGather(callExpr);
}
std::string GatherChannelExtractor::CreateDeclaration(const std::string& callName, const OperationMetadata& metadata)
{
    return this->CreateChannelDeclaration(callName, metadata, "SMI_GatherChannel", "int send_count, int recv_count, SMI_Datatype data_type, int port, int root, SMI_Comm comm");
}
std::string GatherChannelExtractor::GetChannelFunctionName()
{
    return "SMI_Open_gather_channel";
}

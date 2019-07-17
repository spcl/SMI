#include "scatter.h"
#include "utils.h"

using namespace clang;

static OperationMetadata extractScatter(CallExpr* channelDecl)
{
    return OperationMetadata("scatter",
                             extractIntArg(channelDecl, 3),
                             extractDataType(channelDecl, 2),
                             extractBufferSize(channelDecl, 6)
    );
}

OperationMetadata ScatterExtractor::GetOperationMetadata(CallExpr* callExpr)
{
    return extractScatter(extractChannelDecl(callExpr));
}
std::string ScatterExtractor::CreateDeclaration(const std::string& callName, const OperationMetadata& metadata)
{
    return "void " + this->RenameCall(callName, metadata) + "(SMI_ScatterChannel* chan, void* data_snd, void* data_rcv);";
}
std::vector<std::string> ScatterExtractor::GetFunctionNames()
{
    return {"SMI_Scatter"};
}

OperationMetadata ScatterChannelExtractor::GetOperationMetadata(CallExpr* callExpr)
{
    return extractScatter(callExpr);
}
std::string ScatterChannelExtractor::CreateDeclaration(const std::string& callName, const OperationMetadata& metadata)
{
    return this->CreateChannelDeclaration(callName, metadata, "SMI_ScatterChannel", "int send_count, int recv_count, SMI_Datatype data_type, int port, int root, SMI_Comm comm");
}
std::string ScatterChannelExtractor::GetChannelFunctionName()
{
    return "SMI_Open_scatter_channel";
}

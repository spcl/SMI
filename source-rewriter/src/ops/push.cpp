#include "push.h"
#include "utils.h"

using namespace clang;

static OperationMetadata extractPush(CallExpr* channelDecl)
{
    return OperationMetadata("push",
                             extractIntArg(channelDecl, 3),
                             extractDataType(channelDecl, 1),
                             extractBufferSize(channelDecl, 5)
    );
}

OperationMetadata PushExtractor::GetOperationMetadata(CallExpr* callExpr)
{
    return extractPush(extractChannelDecl(callExpr));
}
std::string PushExtractor::CreateDeclaration(const std::string& callName, const OperationMetadata& metadata)
{
    std::string args = "(SMI_Channel* chan, void* data";
    if (callName == "SMI_Push_flush")
    {
        args += ", int immediate";
    }

    return "void " + this->RenameCall(callName, metadata) + args + ");";
}
std::vector<std::string> PushExtractor::GetFunctionNames()
{
    return {"SMI_Push"};
}

OperationMetadata PushChannelExtractor::GetOperationMetadata(CallExpr* callExpr)
{
    return extractPush(callExpr);
}
std::string PushChannelExtractor::CreateDeclaration(const std::string& callName, const OperationMetadata& metadata)
{
    return this->CreateChannelDeclaration(callName, metadata, "SMI_Channel", "int count, SMI_Datatype data_type, int destination, int port, SMI_Comm comm");
}
std::string PushChannelExtractor::GetChannelFunctionName()
{
    return "SMI_Open_send_channel";
}

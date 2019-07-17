#include "reduce.h"
#include "utils.h"

using namespace clang;

static std::string formatReduceOp(int op)
{
    switch (op)
    {
        case 0: return "add";
        case 1: return "max";
        case 2: return "min";
    }

    assert(false);
    return "";
}

static OperationMetadata extractReduce(CallExpr* channelDecl)
{
    return OperationMetadata("reduce",
                             extractIntArg(channelDecl, 3),
                             extractDataType(channelDecl, 1),
                             extractBufferSize(channelDecl, 6),
                             { {"op_type", formatReduceOp(extractIntArg(channelDecl, 2))} }
    );
}

OperationMetadata ReduceExtractor::GetOperationMetadata(CallExpr* callExpr)
{
    return extractReduce(extractChannelDecl(callExpr));
}
std::string ReduceExtractor::CreateDeclaration(const std::string& callName, const OperationMetadata& metadata)
{
    return "void " + this->RenameCall(callName, metadata) + "(SMI_RChannel* chan,  void* data_snd, void* data_rcv);";
}
std::vector<std::string> ReduceExtractor::GetFunctionNames()
{
    return {"SMI_Reduce"};
}

OperationMetadata ReduceChannelExtractor::GetOperationMetadata(CallExpr* callExpr)
{
    return extractReduce(callExpr);
}
std::string ReduceChannelExtractor::CreateDeclaration(const std::string& callName, const OperationMetadata& metadata)
{
    return this->CreateChannelDeclaration(callName, metadata, "SMI_RChannel", "int count, SMI_Datatype data_type, SMI_Op op, int port, int root, SMI_Comm comm");
}
std::string ReduceChannelExtractor::GetChannelFunctionName()
{
    return "SMI_Open_reduce_channel";
}

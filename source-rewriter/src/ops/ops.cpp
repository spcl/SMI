#include "ops.h"
#include "utils.h"

#include "../third-party/json.hpp"

using json = nlohmann::json;

std::string OperationExtractor::RenameCall(const std::string& callName, const OperationMetadata& metadata)
{
    return renamePortDataType(callName, metadata);
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
    ss << returnType << " " << this->RenameCall(callName, metadata) << "(";
    ss << parameters;
    if (isExtendedChannelOpen(callName))
    {
        ss << ", int bufferSize";
    }
    ss << ");";

    return ss.str();
}

std::string ChannelExtractor::RenameCall(const std::string& callName, const OperationMetadata& metadata)
{
    auto name = OperationExtractor::RenameCall(callName, metadata);
    if (isExtendedChannelOpen(name))
    {
        name.resize(name.size() - 3);
    }
    return name;
}

std::vector<std::string> ChannelExtractor::GetFunctionNames()
{
    auto name = this->GetChannelFunctionName();
    return {name, name + "_ad"};
}

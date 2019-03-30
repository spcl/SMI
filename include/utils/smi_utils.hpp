#include <mpi.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>

void checkMpiCall(int code, const char* location, int line)
{
    if (code != MPI_SUCCESS)
    {
        char error[256];
        int length;
        MPI_Error_string(code, error, &length);
        std::cerr << "MPI error at " << location << ":" << line << ": " << error << std::endl;
    }
}

#define CHECK_MPI(err) checkMpiCall((err), __FILE__, __LINE__);

template <typename DataSize>
void load_routing_table(int rank, int channel, int ranks, const std::string& routing_directory,
        const std::string& prefix, DataSize* table)
{
    std::stringstream path;
    path << routing_directory << "/" << prefix << "-rank" << rank << "-channel" << channel;

    std::ifstream file(path.str(), std::ios::binary);
    if (!file)
    {
        std::cerr << "Routing table " << path.str() << " not found" << std::endl;
        std::exit(1);
    }

    auto byte_size = ranks * sizeof(DataSize);
    file.read(table, byte_size);
}

std::string replace(std::string source, const std::string& pattern, const std::string& replacement)
{
    auto pos = source.find(pattern);
    if (pos != std::string::npos)
    {
        return source.substr(0, pos) + replacement + source.substr(pos + pattern.length());
    }
    return source;
}

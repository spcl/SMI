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

constexpr int kChannelsPerRank = 4;

template <typename DataSize>
void LoadRoutingTable(int rank, int channel, int num_entries,
                      const std::string& routing_directory,
                      const std::string& prefix, DataSize* table) {
  std::stringstream path;
  path << routing_directory << "/" << prefix << "-rank" << rank << "-channel"
       << channel;

  std::ifstream file(path.str(), std::ios::binary);
  if (!file) {
    throw std::runtime_error("Routing table " + path.str() + " not found.");
  }

  auto byte_size = num_entries * sizeof(DataSize);
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

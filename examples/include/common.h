#pragma once

#include <fstream>
#include <sstream>
#include <string>

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

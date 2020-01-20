#include "filesystem.hpp"

#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

std::uintmax_t getFileSize(const fs::path &path) {
  std::ifstream ifstream(path, std::ifstream::in | std::ifstream::binary);

  if (!ifstream.is_open()) {
    throw std::runtime_error("Error: Failed to open file.");
  }

  ifstream.seekg(0, std::ifstream::end);
  auto size = ifstream.tellg();

  if (size < 0) {
    throw std::runtime_error("Error: Failed to get file size.");
  }

  return size;
}

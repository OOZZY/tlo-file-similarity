#include "filesystem.hpp"

#include <fstream>

namespace fs = std::filesystem;

std::pair<int, std::uintmax_t> getFileSize(const fs::path &path) {
  std::ifstream ifstream(path, std::ifstream::in | std::ifstream::binary);

  if (!ifstream.is_open()) {
    return std::pair<int, std::uintmax_t>(1, 0);
  }

  ifstream.seekg(0, std::ifstream::end);
  auto size = ifstream.tellg();

  if (size < 0) {
    return std::pair<int, std::uintmax_t>(1, 0);
  }

  return std::pair<int, std::uintmax_t>(0, size);
}

#include "filesystem.hpp"

#include <fstream>
#include <stdexcept>

#include "chrono.hpp"

namespace fs = std::filesystem;

namespace tlo {
std::uintmax_t getFileSize(const fs::path &path) {
  std::ifstream ifstream(path, std::ifstream::in | std::ifstream::binary);

  if (!ifstream.is_open()) {
    throw std::runtime_error("Error: Failed to open \"" +
                             path.generic_string() + "\".");
  }

  ifstream.seekg(0, std::ifstream::end);

  auto size = ifstream.tellg();

  if (size < 0) {
    throw std::runtime_error("Error: Failed to get size of \"" +
                             path.generic_string() + "\".");
  }

  return static_cast<std::uintmax_t>(size);
}

std::time_t getLastWriteTime(const fs::path &path) {
  auto timeOnFileClock = fs::last_write_time(path);
  auto timeOnSystemClock =
      convertTimePoint<std::chrono::system_clock::time_point>(timeOnFileClock);

  return std::chrono::system_clock::to_time_t(timeOnSystemClock);
}
}  // namespace tlo

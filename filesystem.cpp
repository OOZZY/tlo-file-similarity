#include "filesystem.hpp"

#include <fstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include "chrono.hpp"

namespace fs = std::filesystem;

namespace tlo {
std::uintmax_t getFileSize(const fs::path &path) {
  std::ifstream ifstream(path, std::ifstream::in | std::ifstream::binary);

  if (!ifstream.is_open()) {
    throw std::runtime_error("Error: Failed to open \"" + path.string() +
                             "\".");
  }

  ifstream.seekg(0, std::ifstream::end);

  auto size = ifstream.tellg();

  if (size < 0) {
    throw std::runtime_error("Error: Failed to get size of \"" + path.string() +
                             "\".");
  }

  return static_cast<std::uintmax_t>(size);
}

std::time_t getLastWriteTime(const fs::path &path) {
  auto timeOnFileClock = fs::last_write_time(path);
  auto timeOnSystemClock =
      convertTimePoint<std::chrono::system_clock::time_point>(timeOnFileClock);

  return std::chrono::system_clock::to_time_t(timeOnSystemClock);
}

std::size_t HashPath::operator()(const fs::path &path) const {
  return fs::hash_value(path);
}

std::vector<fs::path> stringsToPaths(const std::vector<std::string> &strings) {
  std::vector<fs::path> paths;
  std::unordered_set<fs::path, HashPath> pathsAdded;

  for (const auto &string : strings) {
    fs::path path = string;

    if (!fs::exists(path)) {
      throw std::runtime_error("Error: Path \"" + string +
                               "\" does not exist.");
    }

    fs::path canonicalPath = fs::canonical(path);

    if (pathsAdded.find(canonicalPath) == pathsAdded.end()) {
      paths.push_back(canonicalPath);
      pathsAdded.insert(std::move(canonicalPath));
    }
  }

  return paths;
}
}  // namespace tlo

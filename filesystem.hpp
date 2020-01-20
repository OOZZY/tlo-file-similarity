#ifndef TLOFS_FILESYSTEM_HPP
#define TLOFS_FILESYSTEM_HPP

#include <cstdint>
#include <filesystem>
#include <utility>

// On MinGW-w64, sometimes std::filesystem::file_size() returns the wrong size
// for large files. Returns error code (0 means success) and size
std::pair<int, std::uintmax_t> getFileSize(const std::filesystem::path &path);

#endif  // TLOFS_FILESYSTEM_HPP

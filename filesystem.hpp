#ifndef TLOFS_FILESYSTEM_HPP
#define TLOFS_FILESYSTEM_HPP

#include <cstdint>
#include <filesystem>

// On MinGW-w64, sometimes std::filesystem::file_size() returns the wrong size
// for large files. Returns file size. Throws std::runtime_error on error.
std::uintmax_t getFileSize(const std::filesystem::path &path);

#endif  // TLOFS_FILESYSTEM_HPP

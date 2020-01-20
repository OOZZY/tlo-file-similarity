#ifndef TLOFS_SIMILARITY_HPP
#define TLOFS_SIMILARITY_HPP

#include <cstdint>
#include <filesystem>
#include <utility>

// On MinGW-w64, sometimes std::filesystem::file_size() returns the wrong size
// for large files. Returns error code (0 means success) and size
std::pair<int, std::uintmax_t> getFileSize(const std::filesystem::path &path);

double checkFileSimilarity(const std::filesystem::path &path1,
                           const std::filesystem::path &path2,
                           std::size_t blockSize);

double checkFileSimilarity(const std::filesystem::path &path1,
                           const std::filesystem::path &path2,
                           std::size_t blockSize, size_t numThreads);

#endif  // TLOFS_SIMILARITY_HPP

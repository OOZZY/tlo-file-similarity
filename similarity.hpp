#ifndef TLOFS_SIMILARITY_HPP
#define TLOFS_SIMILARITY_HPP

#include <cstddef>
#include <filesystem>

double checkFileSimilarity(const std::filesystem::path &path1,
                           const std::filesystem::path &path2,
                           std::size_t blockSize);

double checkFileSimilarity(const std::filesystem::path &path1,
                           const std::filesystem::path &path2,
                           std::size_t blockSize, size_t numThreads);

#endif  // TLOFS_SIMILARITY_HPP

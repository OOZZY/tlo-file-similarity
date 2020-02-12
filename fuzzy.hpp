#ifndef TLOFS_FUZZY_HPP
#define TLOFS_FUZZY_HPP

#include <cstddef>
#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

namespace tlo {
struct FuzzyHash {
  std::size_t blockSize = 0;

  // String of Base64 values where each value corresponds to a hash of a block
  // of the file and the block size used is blockSize. Note, the actual size of
  // each block is not necessarily blockSize because block boundaries are
  // determined in a context-dependent manner.
  std::string part1;

  // Similar to part1, except the block size used is 2 * blockSize.
  std::string part2;

  // Path to the file.
  std::string path;
};

std::ostream &operator<<(std::ostream &os, const FuzzyHash &hash);

class FuzzyHashEventHandler {
 public:
  virtual void onBlockHash() = 0;
  virtual void onFileHash(const FuzzyHash &hash) = 0;
  virtual bool shouldHashFile(const std::filesystem::path &filePath) = 0;
  virtual void collect(FuzzyHash &&hash) = 0;
  virtual ~FuzzyHashEventHandler() = 0;
};

// Based on spamsum and ssdeep. Throws std::runtime_error on error. If handler
// is not nullptr, will call handler->onBlockHash() whenever a file block has
// just been hashed. Also, will call handler->onFileHash() whenever a file has
// just been hashed. Expects path to be a path to a file.
FuzzyHash fuzzyHash(const std::filesystem::path &filePath,
                    FuzzyHashEventHandler *handler = nullptr);

// Expects paths to be paths to files or directories. If a path refers to a
// file, will hash the file. If a path refers to a directory, will hash all
// files in the directory and all its subdirectories. If a path is neither a
// file or directory, will throw std::runtime_error. Each file is hashed by
// calling handler.collect(fuzzyHash(path, &handler)). Before hashing a file,
// calls handler.shouldHashFile(path) to check if a file should be hashed. The
// handler can be used to process the hashes. If numThreads > 1, make sure that
// the handler's member functions are synchronized.
void fuzzyHash(const std::vector<std::filesystem::path> &paths,
               FuzzyHashEventHandler &handler, std::size_t numThreads = 1);

// Given string should have the format <blockSize>:<part1>:<part2>,<path>.
// Throws std::runtime_error on error.
FuzzyHash parseHash(const std::string &hash);
}  // namespace tlo

#endif  // TLOFS_FUZZY_HPP

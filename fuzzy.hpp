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
  virtual ~FuzzyHashEventHandler() = 0;
};

// Based on spamsum and ssdeep. Throws std::runtime_error on error. If handler
// is not nullptr, will call handler->onBlockHash() every time a file block has
// finished hashing with whatever the current blockSize is (not 2 * blockSize).
// Also, will call handler->onFileHash() every time a file has finished
// hashing. Expects path to be a path to a file.
FuzzyHash fuzzyHash(const std::filesystem::path &path,
                    FuzzyHashEventHandler *handler = nullptr);

// Expects strings in paths to be paths to files or directories. If a string is
// a path to a file, will hash the file. If a string is a path to a directory,
// will hash all files in the directory and all its subdirectories. If a string
// is neither a file or directory, will throw std::runtime_error. Each file is
// hashed by calling fuzzyHash(path, &handler). The handler can be used to
// process the hashes. If numThreads > 1, make sure that the handler's member
// functions are synchronized.
void fuzzyHash(const std::vector<std::string> &paths,
               FuzzyHashEventHandler &handler, std::size_t numThreads = 1);

// Given string should have the format
// <blockSize>:<part1>:<part2>,<path>. Throws std::runtime_error on
// error.
FuzzyHash parseHash(const std::string &hash);
}  // namespace tlo

#endif  // TLOFS_FUZZY_HPP

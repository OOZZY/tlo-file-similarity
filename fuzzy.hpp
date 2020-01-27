#ifndef TLOFS_FUZZY_HPP
#define TLOFS_FUZZY_HPP

#include <cstddef>
#include <filesystem>
#include <ostream>
#include <string>

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
};

// Based on spamsum and ssdeep. Throws std::runtime_error on error. If handler
// is not nullptr, will call handler->onBlockHash() every time a file block has
// finished hashing with whatever the current blockSize is (not 2 * blockSize).
// Also, will call handler->onFileHash() every time a file has finished
// hashing.
FuzzyHash fuzzyHash(const std::filesystem::path &path,
                    FuzzyHashEventHandler *handler = nullptr);

// Given string should have the format
// <blockSize>:<part1>:<part2>,<path>. Throws std::runtime_error on
// error.
FuzzyHash parseHash(const std::string &hash);

bool hashesAreComparable(const FuzzyHash &hash1, const FuzzyHash &hash2);

// Returns score from 0 to 100 of how similar the given hashes are. A score
// closer to 100 means the hashes are more similar. Throws std::runtime_error
// if hashes are not comparable.
double compareHashes(const FuzzyHash &hash1, const FuzzyHash &hash2);
}  // namespace tlo

#endif  // TLOFS_FUZZY_HPP

#ifndef TLOFS_FUZZY_HPP
#define TLOFS_FUZZY_HPP

#include <cstddef>
#include <filesystem>
#include <ostream>
#include <string>

struct FuzzyHashResult {
  std::size_t blockSize = 0;

  // Hash when block size was blockSize.
  std::string signature1;

  // Hash when block size was 2 * blockSize.
  std::string signature2;

  std::string path;
};

std::ostream &operator<<(std::ostream &os, const FuzzyHashResult &result);

// Based on spamsum and ssdeep. Throws std::runtime_error on error.
FuzzyHashResult fuzzyHash(const std::filesystem::path &path);

// Given string should have the format
// <blockSize>:<signature1>:<signature2>,<path>. Throws std::runtime_error on
// error.
FuzzyHashResult parseHash(const std::string &hash);

bool hashesAreComparable(const FuzzyHashResult &result1,
                         const FuzzyHashResult &result2);

// Returns score from 0 to 100 of how similar the given hashes are. A score
// closer to 100 means the hashes are more similar. Throws std::runtime_error
// if hashes are not comparable.
double compareHashes(const FuzzyHashResult &result1,
                     const FuzzyHashResult &result2);

#endif  // TLOFS_FUZZY_HPP

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
};

std::ostream &operator<<(std::ostream &os, const FuzzyHashResult &result);

// Based on spamsum and ssdeep. Throws std::runtime_error on error.
FuzzyHashResult fuzzyHash(const std::filesystem::path &path);

#endif  // TLOFS_FUZZY_HPP

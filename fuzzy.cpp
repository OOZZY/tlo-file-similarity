#include "fuzzy.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>

#include "filesystem.hpp"
#include "lcs.hpp"
#include "string.hpp"

namespace fs = std::filesystem;

namespace tlo {
std::ostream &operator<<(std::ostream &os, const FuzzyHash &hash) {
  return os << hash.blockSize << ':' << hash.part1 << ':' << hash.part2 << ','
            << hash.path;
}

/*
 * Algorithms from:
 * https://www.dfrws.org/sites/default/files/session-files/paper-identifying_almost_identical_files_using_context_triggered_piecewise_hashing.pdf
 * https://github.com/ssdeep-project/ssdeep/blob/master/fuzzy.c
 * http://www.isthe.com/chongo/tech/comp/fnv/
 */

constexpr std::size_t WINDOW_SIZE = 7;

class RollingHasher {
 private:
  uint32_t x = 0;
  uint32_t y = 0;
  uint32_t z = 0;
  uint32_t c = 0;
  uint32_t window[WINDOW_SIZE] = {0};
  bool bytesWereAdded_ = false;

 public:
  void addByte(unsigned char byte) {
    y -= x;
    y += WINDOW_SIZE * byte;
    x += byte;
    x -= window[c % WINDOW_SIZE];
    window[c % WINDOW_SIZE] = byte;
    c++;
    z <<= 5;
    z ^= byte;
    bytesWereAdded_ = true;
  }

  uint32_t getHash() const { return x + y + z; }

  bool bytesWereAdded() { return bytesWereAdded_; }
};

constexpr uint32_t OFFSET_BASIS = 2166136261U;
constexpr uint32_t FNV_PRIME = 16777619U;

class FNV1Hasher {
 private:
  uint32_t hash = OFFSET_BASIS;
  bool bytesWereAdded_ = false;

 public:
  void addByte(unsigned char byte) {
    hash = (hash * FNV_PRIME) ^ byte;
    bytesWereAdded_ = true;
  }

  uint32_t getHash() const { return hash; }

  bool bytesWereAdded() { return bytesWereAdded_; }
};

constexpr std::size_t MIN_BLOCK_SIZE = 3;
constexpr std::size_t SPAMSUM_LENGTH = 64;
constexpr std::size_t BUFFER_SIZE = 1000000;
constexpr std::string_view BASE64_ALPHABET =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::pair<std::string, std::string> fuzzyHash(const fs::path &file,
                                              std::size_t blockSize) {
  std::ifstream ifstream(file, std::ifstream::in | std::ifstream::binary);

  if (!ifstream.is_open()) {
    throw std::runtime_error("Error: Failed to open \"" +
                             file.generic_string() + "\".");
  }

  std::vector<char> buffer(BUFFER_SIZE, 0);
  RollingHasher rollingHasher;
  FNV1Hasher fnv1Hasher1;
  FNV1Hasher fnv1Hasher2;
  std::string part1;
  std::string part2;

  while (!ifstream.eof()) {
    ifstream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    std::size_t numCharsRead = static_cast<std::size_t>(ifstream.gcount());

    for (std::size_t i = 0; i < numCharsRead; ++i) {
      unsigned char byte = static_cast<unsigned char>(buffer[i]);
      rollingHasher.addByte(byte);
      fnv1Hasher1.addByte(byte);
      fnv1Hasher2.addByte(byte);

      if (rollingHasher.getHash() % blockSize == blockSize - 1) {
        part1 +=
            BASE64_ALPHABET[fnv1Hasher1.getHash() % BASE64_ALPHABET.size()];
        fnv1Hasher1 = FNV1Hasher();
      }

      if (rollingHasher.getHash() % (blockSize * 2) == blockSize * 2 - 1) {
        part2 +=
            BASE64_ALPHABET[fnv1Hasher2.getHash() % BASE64_ALPHABET.size()];
        fnv1Hasher2 = FNV1Hasher();
      }
    }
  }

  if (fnv1Hasher1.bytesWereAdded()) {
    part1 += BASE64_ALPHABET[fnv1Hasher1.getHash() % BASE64_ALPHABET.size()];
    fnv1Hasher1 = FNV1Hasher();
  }

  if (fnv1Hasher2.bytesWereAdded()) {
    part2 += BASE64_ALPHABET[fnv1Hasher2.getHash() % BASE64_ALPHABET.size()];
    fnv1Hasher2 = FNV1Hasher();
  }

  return std::pair(part1, part2);
}

FuzzyHash fuzzyHash(const fs::path &path) {
  if (!fs::is_regular_file(path)) {
    throw std::runtime_error("Error: \"" + path.generic_string() +
                             "\" is not a file.");
  }

  auto fileSize = getFileSize(path);
  if (fileSize == 0) {
    return {MIN_BLOCK_SIZE, "", "", path.generic_string()};
  }

  double exponent = std::floor(std::log2(static_cast<double>(fileSize) /
                                         (SPAMSUM_LENGTH * MIN_BLOCK_SIZE)));
  std::size_t blockSize = static_cast<std::size_t>(
      std::ceil(MIN_BLOCK_SIZE * std::pow(2, exponent)));

  if (blockSize < MIN_BLOCK_SIZE) {
    blockSize = MIN_BLOCK_SIZE;
  }

  std::string part1;
  std::string part2;

  for (;;) {
    std::tie(part1, part2) = fuzzyHash(path, blockSize);

    if (part1.size() < SPAMSUM_LENGTH / 2 && blockSize / 2 >= MIN_BLOCK_SIZE) {
      blockSize /= 2;
    } else {
      break;
    }
  }

  return {blockSize, part1, part2, path.generic_string()};
}

FuzzyHash parseHash(const std::string &hash) {
  std::vector<std::string> commaSplit = split(hash, ',');

  if (commaSplit.size() < 2) {
    throw std::runtime_error("Error: \"" + hash + "\" does not have a comma.");
  }

  std::vector<std::string> colonSplit = split(commaSplit[0], ':');

  if (colonSplit.size() != 3) {
    throw std::runtime_error(
        "Error: \"" + hash +
        "\" has the wrong number of sections separated by a colon.");
  }

  return {std::stoull(colonSplit[0]), colonSplit[1], colonSplit[2],
          commaSplit[1]};
}

bool hashesAreComparable(const FuzzyHash &hash1, const FuzzyHash &hash2) {
  if (hash1.blockSize == hash2.blockSize) {
    return true;
  } else if (hash1.blockSize == 2 * hash2.blockSize) {
    return true;
  } else if (2 * hash1.blockSize == hash2.blockSize) {
    return true;
  } else {
    return false;
  }
}

namespace {
double calculateSimilarity(const std::string &string1,
                           const std::string &string2) {
  auto lcsDistance = lcsLength3(string1, string2).lcsDistance;
  auto maxLCSDistance = ::tlo::maxLCSDistance(string1.size(), string2.size());
  return static_cast<double>(maxLCSDistance - lcsDistance) / maxLCSDistance *
         100.0;
}
}  // namespace

double compareHashes(const FuzzyHash &hash1, const FuzzyHash &hash2) {
  if (hash1.blockSize == hash2.blockSize) {
    double part1Similarity = calculateSimilarity(hash1.part1, hash2.part1);
    double part2Similarity = calculateSimilarity(hash1.part2, hash2.part2);
    return std::max(part1Similarity, part2Similarity);
  } else if (hash1.blockSize == 2 * hash2.blockSize) {
    double partSimilarity = calculateSimilarity(hash1.part1, hash2.part2);
    return partSimilarity;
  } else if (2 * hash1.blockSize == hash2.blockSize) {
    double partSimilarity = calculateSimilarity(hash1.part2, hash2.part1);
    return partSimilarity;
  } else {
    throw std::runtime_error("Error: \"" + toString(hash1) + "\" and \"" +
                             toString(hash2) + "\" are not comparable.");
  }
}
}  // namespace tlo

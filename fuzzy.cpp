#include "fuzzy.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string_view>

#include "filesystem.hpp"
#include "lcs.hpp"
#include "string.hpp"

namespace fs = std::filesystem;

std::ostream &operator<<(std::ostream &os, const FuzzyHashResult &result) {
  return os << result.blockSize << ':' << result.signature1 << ':'
            << result.signature2 << ',' << result.path;
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

FuzzyHashResult fuzzyHash(const std::filesystem::path &path) {
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

  std::string signature1;
  std::string signature2;

  for (;;) {
    RollingHasher rollingHasher;
    FNV1Hasher fnv1Hasher1;
    FNV1Hasher fnv1Hasher2;

    signature1.clear();
    signature2.clear();

    std::ifstream ifstream(path, std::ifstream::in | std::ifstream::binary);
    std::vector<char> buffer(BUFFER_SIZE, 0);

    if (!ifstream.is_open()) {
      throw std::runtime_error("Error: Failed to open \"" +
                               path.generic_string() + "\".");
    }

    while (!ifstream.eof()) {
      ifstream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
      std::size_t numCharsRead = static_cast<std::size_t>(ifstream.gcount());

      for (std::size_t i = 0; i < numCharsRead; ++i) {
        rollingHasher.addByte(buffer[i]);
        fnv1Hasher1.addByte(buffer[i]);
        fnv1Hasher2.addByte(buffer[i]);

        if (rollingHasher.getHash() % blockSize == blockSize - 1) {
          signature1 +=
              BASE64_ALPHABET[fnv1Hasher1.getHash() % BASE64_ALPHABET.size()];
          fnv1Hasher1 = FNV1Hasher();
        }

        if (rollingHasher.getHash() % (blockSize * 2) == blockSize * 2 - 1) {
          signature2 +=
              BASE64_ALPHABET[fnv1Hasher2.getHash() % BASE64_ALPHABET.size()];
          fnv1Hasher2 = FNV1Hasher();
        }
      }
    }

    if (fnv1Hasher1.bytesWereAdded()) {
      signature1 +=
          BASE64_ALPHABET[fnv1Hasher1.getHash() % BASE64_ALPHABET.size()];
      fnv1Hasher1 = FNV1Hasher();
    }

    if (fnv1Hasher2.bytesWereAdded()) {
      signature2 +=
          BASE64_ALPHABET[fnv1Hasher2.getHash() % BASE64_ALPHABET.size()];
      fnv1Hasher2 = FNV1Hasher();
    }

    if (signature1.size() < SPAMSUM_LENGTH / 2 &&
        blockSize / 2 >= MIN_BLOCK_SIZE) {
      blockSize /= 2;
    } else {
      break;
    }
  }

  return {blockSize, signature1, signature2, path.generic_string()};
}

FuzzyHashResult parseHash(const std::string &hash) {
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

bool hashesAreComparable(const FuzzyHashResult &result1,
                         const FuzzyHashResult &result2) {
  if (result1.blockSize == result2.blockSize) {
    return true;
  } else if (result1.blockSize == 2 * result2.blockSize) {
    return true;
  } else if (2 * result1.blockSize == result2.blockSize) {
    return true;
  } else {
    return false;
  }
}

namespace {
double calculateSimilarity(const std::string &string1,
                           const std::string &string2) {
  auto lcsLength =
      lcsLength3(string1, string2, lcsLength2_<std::string>).lcsLength;
  auto largerSize = std::max(string1.size(), string2.size());
  return static_cast<double>(lcsLength) / largerSize * 100;
}
}  // namespace

double compareHashes(const FuzzyHashResult &result1,
                     const FuzzyHashResult &result2) {
  if (result1.blockSize == result2.blockSize) {
    double signature1Similarity =
        calculateSimilarity(result1.signature1, result2.signature1);
    double signature2Similarity =
        calculateSimilarity(result1.signature2, result2.signature2);
    return std::max(signature1Similarity, signature2Similarity);
  } else if (result1.blockSize == 2 * result2.blockSize) {
    double signatureSimilarity =
        calculateSimilarity(result1.signature1, result2.signature2);
    return signatureSimilarity;
  } else if (2 * result1.blockSize == result2.blockSize) {
    double signatureSimilarity =
        calculateSimilarity(result1.signature2, result2.signature1);
    return signatureSimilarity;
  } else {
    throw std::runtime_error("Error: \"" + toString(result1) + "\" and \"" +
                             toString(result2) + "\" are not comparable.");
  }
}

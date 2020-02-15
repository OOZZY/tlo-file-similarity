#include "compare.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <utility>

#include "damerau-levenshtein.hpp"
#include "lcs.hpp"
#include "levenshtein.hpp"
#include "string.hpp"

namespace fs = std::filesystem;

namespace tlo {
double compareWithLcsDistance(const std::string &string1,
                              const std::string &string2) {
  auto lcsDistance = lcsLength3(string1, string2).lcsDistance;
  auto maxLcsDistance = ::tlo::maxLcsDistance(string1.size(), string2.size());

  if (maxLcsDistance == 0) {
    return 100.0;
  }

  return static_cast<double>(maxLcsDistance - lcsDistance) / maxLcsDistance *
         100.0;
}

double compareWithLevenshteinDistance(const std::string &string1,
                                      const std::string &string2) {
  auto levenshteinDistance = levenshteinDistance3(string1, string2);
  auto maxLevenshteinDistance =
      ::tlo::maxLevenshteinDistance(string1.size(), string2.size());

  if (maxLevenshteinDistance == 0) {
    return 100.0;
  }

  return static_cast<double>(maxLevenshteinDistance - levenshteinDistance) /
         maxLevenshteinDistance * 100.0;
}

double compareWithDamerLevenDistance(const std::string &string1,
                                     const std::string &string2) {
  auto damerLevenDistance = damerLevenDistance2(string1, string2);
  auto maxDamerLevenDistance =
      ::tlo::maxDamerLevenDistance(string1.size(), string2.size());

  if (maxDamerLevenDistance == 0) {
    return 100.0;
  }

  return static_cast<double>(maxDamerLevenDistance - damerLevenDistance) /
         maxDamerLevenDistance * 100.0;
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

double compareHashes(const FuzzyHash &hash1, const FuzzyHash &hash2) {
  if (hash1.blockSize == hash2.blockSize) {
    double part1Similarity = compareWithLcsDistance(hash1.part1, hash2.part1);
    double part2Similarity = compareWithLcsDistance(hash1.part2, hash2.part2);

    return std::max(part1Similarity, part2Similarity);
  } else if (hash1.blockSize == 2 * hash2.blockSize) {
    return compareWithLcsDistance(hash1.part1, hash2.part2);
  } else if (2 * hash1.blockSize == hash2.blockSize) {
    return compareWithLcsDistance(hash1.part2, hash2.part1);
  } else {
    throw std::runtime_error("Error: \"" + toString(hash1) + "\" and \"" +
                             toString(hash2) + "\" are not comparable.");
  }
}

namespace {
void readHashesFromFile(
    std::unordered_map<std::size_t, std::vector<FuzzyHash>> &blockSizesToHashes,
    std::unordered_set<FuzzyHash, HashFuzzyHash> &hashesAdded,
    const fs::path &textFilePath) {
  std::ifstream ifstream(textFilePath, std::ifstream::in);

  if (!ifstream.is_open()) {
    throw std::runtime_error("Error: Failed to open \"" +
                             textFilePath.string() + "\".");
  }

  std::string line;

  while (std::getline(ifstream, line)) {
    FuzzyHash hash = parseHash(line);

    if (hashesAdded.find(hash) == hashesAdded.end()) {
      blockSizesToHashes[hash.blockSize].push_back(hash);
      hashesAdded.insert(std::move(hash));
    }
  }
}
}  // namespace

std::unordered_map<std::size_t, std::vector<FuzzyHash>> readHashes(
    const std::vector<fs::path> &textFilePaths) {
  for (const auto &textFilePath : textFilePaths) {
    if (!fs::is_regular_file(textFilePath)) {
      throw std::runtime_error("Error: \"" + textFilePath.string() +
                               "\" is not a file.");
    }
  }

  std::unordered_map<std::size_t, std::vector<FuzzyHash>> blockSizesToHashes;
  std::unordered_set<FuzzyHash, HashFuzzyHash> hashesAdded;

  for (const auto &textFilePath : textFilePaths) {
    readHashesFromFile(blockSizesToHashes, hashesAdded, textFilePath);
  }

  return blockSizesToHashes;
}

HashComparisonEventHandler::~HashComparisonEventHandler() {}

namespace {
// Compare hash with hashes[startIndex .. end].
void compareHashWithOthers(const FuzzyHash &hash,
                           const std::vector<FuzzyHash> &hashes,
                           std::size_t startIndex, int similarityThreshold,
                           HashComparisonEventHandler &handler) {
  for (std::size_t j = startIndex; j < hashes.size(); ++j) {
    if (hashesAreComparable(hash, hashes[j])) {
      double similarityScore = compareHashes(hash, hashes[j]);

      if (similarityScore >= similarityThreshold) {
        handler.onSimilarPairFound(hash, hashes[j], similarityScore);
      }
    }
  }
}

void compareHashesWithSingleThread(
    const std::unordered_map<std::size_t, std::vector<FuzzyHash>>
        &blockSizesToHashes,
    int similarityThreshold, HashComparisonEventHandler &handler) {
  std::vector<std::size_t> blockSizes;

  for (const auto &pair : blockSizesToHashes) {
    blockSizes.push_back(pair.first);
  }

  std::sort(blockSizes.begin(), blockSizes.end());

  for (const auto blockSize : blockSizes) {
    const std::vector<FuzzyHash> &hashes = blockSizesToHashes.at(blockSize);
    const std::vector<FuzzyHash> *moreHashes = nullptr;
    const auto iterator = blockSizesToHashes.find(2 * blockSize);

    if (iterator != blockSizesToHashes.end()) {
      moreHashes = &iterator->second;
    }

    for (std::size_t i = 0; i < hashes.size(); ++i) {
      compareHashWithOthers(hashes[i], hashes, i + 1, similarityThreshold,
                            handler);

      if (moreHashes) {
        compareHashWithOthers(hashes[i], *moreHashes, 0, similarityThreshold,
                              handler);
      }

      handler.onHashDone();
    }
  }
}

struct SharedState {
  const std::vector<std::size_t> &blockSizes;
  const std::unordered_map<std::size_t, std::vector<FuzzyHash>>
      &blockSizesToHashes;
  const int similarityThreshold;

  std::mutex indexMutex;
  bool exceptionThrown = false;
  std::size_t blockSizeIndex = 0;
  std::size_t hashIndex = 0;

  SharedState(const std::vector<std::size_t> &blockSizes_,
              const std::unordered_map<std::size_t, std::vector<FuzzyHash>>
                  &blockSizesToHashes_,
              int similarityThreshold_)
      : blockSizes(blockSizes_),
        blockSizesToHashes(blockSizesToHashes_),
        similarityThreshold(similarityThreshold_) {}
};

void compareHashAtIndexWithComparableHashes(SharedState &state,
                                            HashComparisonEventHandler &handler,
                                            std::exception_ptr &exception) {
  try {
    for (;;) {
      std::unique_lock<std::mutex> indexUniqueLock(state.indexMutex);

      if (state.exceptionThrown) {
        break;
      }

      if (state.blockSizeIndex >= state.blockSizes.size()) {
        break;
      }

      const std::size_t blockSize = state.blockSizes[state.blockSizeIndex];
      const std::vector<FuzzyHash> &hashes =
          state.blockSizesToHashes.at(blockSize);
      const std::size_t i = state.hashIndex;

      state.hashIndex++;

      if (state.hashIndex >= hashes.size()) {
        state.blockSizeIndex++;
        state.hashIndex = 0;
      }

      indexUniqueLock.unlock();

      const std::vector<FuzzyHash> *moreHashes = nullptr;
      const auto iterator = state.blockSizesToHashes.find(2 * blockSize);

      if (iterator != state.blockSizesToHashes.end()) {
        moreHashes = &iterator->second;
      }

      compareHashWithOthers(hashes[i], hashes, i + 1, state.similarityThreshold,
                            handler);

      if (moreHashes) {
        compareHashWithOthers(hashes[i], *moreHashes, 0,
                              state.similarityThreshold, handler);
      }

      handler.onHashDone();
    }
  } catch (...) {
    std::lock_guard<std::mutex> indexLockGuard(state.indexMutex);

    state.exceptionThrown = true;
    exception = std::current_exception();
  }
}

void compareHashesWithMultipleThreads(
    const std::unordered_map<std::size_t, std::vector<FuzzyHash>>
        &blockSizesToHashes,
    int similarityThreshold, HashComparisonEventHandler &handler,
    std::size_t numThreads) {
  std::vector<std::size_t> blockSizes;

  for (const auto &pair : blockSizesToHashes) {
    blockSizes.push_back(pair.first);
  }

  std::sort(blockSizes.begin(), blockSizes.end());

  SharedState state(blockSizes, blockSizesToHashes, similarityThreshold);
  std::vector<std::exception_ptr> exceptions(numThreads);
  std::vector<std::thread> threads(numThreads - 1);

  for (std::size_t i = 0; i < threads.size(); ++i) {
    threads[i] =
        std::thread(compareHashAtIndexWithComparableHashes, std::ref(state),
                    std::ref(handler), std::ref(exceptions[i + 1]));
  }

  compareHashAtIndexWithComparableHashes(state, handler, exceptions[0]);

  for (auto &thread : threads) {
    thread.join();
  }

  for (const auto &exception : exceptions) {
    if (exception) {
      std::rethrow_exception(exception);
    }
  }
}
}  // namespace

void compareHashes(const std::unordered_map<std::size_t, std::vector<FuzzyHash>>
                       &blockSizesToHashes,
                   int similarityThreshold, HashComparisonEventHandler &handler,
                   std::size_t numThreads) {
  if (numThreads <= 1) {
    compareHashesWithSingleThread(blockSizesToHashes, similarityThreshold,
                                  handler);
  } else {
    compareHashesWithMultipleThreads(blockSizesToHashes, similarityThreshold,
                                     handler, numThreads);
  }
}
}  // namespace tlo

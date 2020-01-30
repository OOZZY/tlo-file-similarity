#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fuzzy.hpp"
#include "options.hpp"

namespace fs = std::filesystem;

namespace {
void readHashesFromFile(
    std::unordered_map<std::size_t, std::vector<tlo::FuzzyHash>>
        &blockSizesToHashes,
    const fs::path &file) {
  std::ifstream ifstream(file, std::ifstream::in);
  std::string line;

  while (std::getline(ifstream, line)) {
    try {
      tlo::FuzzyHash hash = tlo::parseHash(line);
      blockSizesToHashes[hash.blockSize].push_back(std::move(hash));
    } catch (const std::exception &exception) {
      std::cerr << exception.what() << std::endl;
    }
  }
}

std::unordered_map<std::size_t, std::vector<tlo::FuzzyHash>> readHashes(
    const std::vector<std::string> &arguments) {
  std::unordered_map<std::size_t, std::vector<tlo::FuzzyHash>>
      blockSizesToHashes;

  for (std::size_t i = 0; i < arguments.size(); ++i) {
    fs::path path = arguments[i];

    if (!fs::is_regular_file(path)) {
      std::cerr << "Error: \"" << path.generic_string() << "\" is not a file."
                << std::endl;
      continue;
    }

    readHashesFromFile(blockSizesToHashes, path);
  }

  return blockSizesToHashes;
}

void printStatus(std::size_t numHashesDone, std::size_t numSimilarPairs) {
  std::cerr << "Done with " << numHashesDone;

  if (numHashesDone == 1) {
    std::cerr << " hash.";
  } else {
    std::cerr << " hashes.";
  }

  std::cerr << " Found " << numSimilarPairs << " similar ";

  if (numSimilarPairs == 1) {
    std::cerr << "pair.";
  } else {
    std::cerr << "pairs.";
  }

  std::cerr << std::endl;
}

void printSimilarPair(const tlo::FuzzyHash &hash1, const tlo::FuzzyHash &hash2,
                      double similarityScore) {
  std::cout << '"' << hash1.path << "\" and \"" << hash2.path << "\" are about "
            << similarityScore << "% similar." << std::endl;
}

// Compare hash with hashes[startIndex .. end]. Returns number of similar pairs
// found (pairs that have a similarity score above similarityThreshold).
std::size_t compareHashWithOthers(const tlo::FuzzyHash &hash,
                                  const std::vector<tlo::FuzzyHash> &hashes,
                                  std::size_t startIndex,
                                  int similarityThreshold,
                                  std::mutex *outputMutex = nullptr) {
  std::size_t numSimilarPairs = 0;

  for (std::size_t j = startIndex; j < hashes.size(); ++j) {
    if (tlo::hashesAreComparable(hash, hashes[j])) {
      double similarityScore = compareHashes(hash, hashes[j]);

      if (similarityScore >= similarityThreshold) {
        numSimilarPairs++;

        auto outputUniqueLock = outputMutex
                                    ? std::unique_lock<std::mutex>(*outputMutex)
                                    : std::unique_lock<std::mutex>();

        printSimilarPair(hash, hashes[j], similarityScore);
      }
    }
  }

  return numSimilarPairs;
}

void compareHashes(
    const std::unordered_map<std::size_t, std::vector<tlo::FuzzyHash>>
        &blockSizesToHashes,
    int similarityThreshold, bool printStatus) {
  std::vector<std::size_t> blockSizes;

  for (const auto &pair : blockSizesToHashes) {
    blockSizes.push_back(pair.first);
  }

  std::sort(blockSizes.begin(), blockSizes.end());

  std::size_t numHashesDone = 0;
  std::size_t numSimilarPairs = 0;

  for (const auto blockSize : blockSizes) {
    const std::vector<tlo::FuzzyHash> &hashes =
        blockSizesToHashes.at(blockSize);
    const std::vector<tlo::FuzzyHash> *moreHashes = nullptr;
    const auto iterator = blockSizesToHashes.find(2 * blockSize);

    if (iterator != blockSizesToHashes.end()) {
      moreHashes = &iterator->second;
    }

    for (std::size_t i = 0; i < hashes.size(); ++i) {
      numSimilarPairs +=
          compareHashWithOthers(hashes[i], hashes, i + 1, similarityThreshold);

      if (moreHashes) {
        numSimilarPairs += compareHashWithOthers(hashes[i], *moreHashes, 0,
                                                 similarityThreshold);
      }

      if (printStatus) {
        numHashesDone++;
        ::printStatus(numHashesDone, numSimilarPairs);
      }
    }
  }
}

struct SharedState {
  std::mutex indexMutex;
  std::size_t blockSizeIndex = 0;
  std::size_t hashIndex = 0;

  std::mutex outputMutex;
  std::size_t numHashesDone = 0;
  std::size_t numSimilarPairs = 0;

  const std::vector<std::size_t> blockSizes;
  const std::unordered_map<std::size_t, std::vector<tlo::FuzzyHash>>
      &blockSizesToHashes;
  const int similarityThreshold;
  const bool printStatus;

  SharedState(const std::vector<std::size_t> &blockSizes_,
              const std::unordered_map<std::size_t, std::vector<tlo::FuzzyHash>>
                  &blockSizesToHashes_,
              int similarityThreshold_, bool printStatus_)
      : blockSizes(blockSizes_),
        blockSizesToHashes(blockSizesToHashes_),
        similarityThreshold(similarityThreshold_),
        printStatus(printStatus_) {}
};

void compareHashAtIndexWithComparableHashes(SharedState &state) {
  for (;;) {
    std::unique_lock<std::mutex> indexUniqueLock(state.indexMutex);

    if (state.blockSizeIndex >= state.blockSizes.size()) {
      break;
    }

    const std::size_t blockSize = state.blockSizes[state.blockSizeIndex];
    const std::vector<tlo::FuzzyHash> &hashes =
        state.blockSizesToHashes.at(blockSize);
    const std::size_t i = state.hashIndex;

    state.hashIndex++;

    if (state.hashIndex >= hashes.size()) {
      state.blockSizeIndex++;
      state.hashIndex = 0;
    }

    indexUniqueLock.unlock();

    const std::vector<tlo::FuzzyHash> *moreHashes = nullptr;
    const auto iterator = state.blockSizesToHashes.find(2 * blockSize);

    if (iterator != state.blockSizesToHashes.end()) {
      moreHashes = &iterator->second;
    }

    std::size_t numSimilarPairs = 0;

    numSimilarPairs +=
        compareHashWithOthers(hashes[i], hashes, i + 1,
                              state.similarityThreshold, &state.outputMutex);

    if (moreHashes) {
      numSimilarPairs +=
          compareHashWithOthers(hashes[i], *moreHashes, 0,
                                state.similarityThreshold, &state.outputMutex);
    }

    if (state.printStatus) {
      const std::lock_guard<std::mutex> outputLockGuard(state.outputMutex);

      state.numSimilarPairs += numSimilarPairs;
      state.numHashesDone++;
      printStatus(state.numHashesDone, state.numSimilarPairs);
    }
  }
}

// numThreads includes main thread.
void compareHashes(
    const std::unordered_map<std::size_t, std::vector<tlo::FuzzyHash>>
        &blockSizesToHashes,
    int similarityThreshold, std::size_t numThreads, bool printStatus) {
  if (numThreads <= 1) {
    compareHashes(blockSizesToHashes, similarityThreshold, printStatus);
    return;
  }

  std::vector<std::size_t> blockSizes;

  for (const auto &pair : blockSizesToHashes) {
    blockSizes.push_back(pair.first);
  }

  std::sort(blockSizes.begin(), blockSizes.end());

  SharedState state(blockSizes, blockSizesToHashes, similarityThreshold,
                    printStatus);
  std::vector<std::thread> threads(numThreads - 1);

  for (auto &thread : threads) {
    thread =
        std::thread(compareHashAtIndexWithComparableHashes, std::ref(state));
  }

  compareHashAtIndexWithComparableHashes(state);

  for (auto &thread : threads) {
    thread.join();
  }
}
}  // namespace

constexpr int DEFAULT_SIMILARITY_THRESHOLD = 50;
constexpr int MIN_SIMILARITY_THRESHOLD = 0;
constexpr int MAX_SIMILARITY_THRESHOLD = 99;

constexpr std::size_t DEFAULT_NUM_THREADS = 1;
constexpr std::size_t MIN_NUM_THREADS = 1;
constexpr std::size_t MAX_NUM_THREADS = 256;

const std::unordered_map<std::string, tlo::OptionAttributes> validOptions{
    {"--similarity-threshold",
     {true,
      "Display only the file pairs with a similarity score greater than or "
      "equal to this threshold (default: " +
          std::to_string(DEFAULT_SIMILARITY_THRESHOLD) + ")."}},
    {"--num-threads",
     {true, "Number of threads the program will use (default: " +
                std::to_string(DEFAULT_NUM_THREADS) + ")."}},
    {"--print-status",
     {false,
      "Allow program to print status updates to stderr (default: off)."}}};

int main(int argc, char **argv) {
  try {
    const tlo::CommandLine commandLine(argc, argv, validOptions);
    if (commandLine.arguments().empty()) {
      std::cerr << "Usage: " << commandLine.program()
                << " [options] <text file with hashes>..." << std::endl;
      commandLine.printValidOptions(std::cerr);
      return 1;
    }

    int similarityThreshold = DEFAULT_SIMILARITY_THRESHOLD;
    std::size_t numThreads = DEFAULT_NUM_THREADS;
    bool printStatus = false;

    if (commandLine.specifiedOption("--similarity-threshold")) {
      similarityThreshold = commandLine.getOptionValueAsInt(
          "--similarity-threshold", MIN_SIMILARITY_THRESHOLD,
          MAX_SIMILARITY_THRESHOLD);
    }

    if (commandLine.specifiedOption("--num-threads")) {
      numThreads = commandLine.getOptionValueAsULong(
          "--num-threads", MIN_NUM_THREADS, MAX_NUM_THREADS);
    }

    if (commandLine.specifiedOption("--print-status")) {
      printStatus = true;
    }

    if (printStatus) {
      std::cerr << "Reading hashes." << std::endl;
    }

    std::unordered_map<std::size_t, std::vector<tlo::FuzzyHash>>
        blockSizesToHashes = readHashes(commandLine.arguments());

    if (printStatus) {
      std::cerr << "Comparing hashes." << std::endl;
    }

    compareHashes(blockSizesToHashes, similarityThreshold, numThreads,
                  printStatus);
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;
    return 1;
  }
}

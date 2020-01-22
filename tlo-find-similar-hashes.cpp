#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "fuzzy.hpp"
#include "options.hpp"

namespace fs = std::filesystem;

void readHashesFromFile(std::vector<FuzzyHash> &hashes, const fs::path &path) {
  std::ifstream ifstream(path, std::ifstream::in);
  std::string line;

  while (std::getline(ifstream, line)) {
    try {
      hashes.push_back(parseHash(line));
    } catch (const std::exception &exception) {
      std::cerr << exception.what() << std::endl;
    }
  }
}

void readHashes(std::vector<FuzzyHash> &hashes,
                const std::vector<std::string> &arguments) {
  for (std::size_t i = 0; i < arguments.size(); ++i) {
    fs::path path = arguments[i];

    if (!fs::is_regular_file(path)) {
      std::cerr << "Error: \"" << path.generic_string() << "\" is not a file."
                << std::endl;
      continue;
    }

    readHashesFromFile(hashes, path);
  }
}

void compareHashes(const std::vector<FuzzyHash> &hashes,
                   int similarityThreshold) {
  for (std::size_t i = 0; i < hashes.size(); ++i) {
    for (std::size_t j = i + 1; j < hashes.size(); ++j) {
      if (hashesAreComparable(hashes[i], hashes[j])) {
        double similarityScore = compareHashes(hashes[i], hashes[j]);

        if (similarityScore >= similarityThreshold) {
          std::cout << '"' << hashes[i].path << "\" and \"" << hashes[j].path
                    << "\" are about " << similarityScore << "% similar."
                    << std::endl;
        }
      }
    }
  }
}

struct SharedState {
  std::mutex indexMutex;
  std::size_t index = 0;

  std::mutex coutMutex;

  const std::vector<FuzzyHash> &hashes;
  const int similarityThreshold;

  SharedState(const std::vector<FuzzyHash> &hashes_, int similarityThreshold_)
      : hashes(hashes_), similarityThreshold(similarityThreshold_) {}
};

void compareHashAtIndexWithAllSubsequentHashes(SharedState &state) {
  for (;;) {
    std::unique_lock<std::mutex> indexUniqueLock(state.indexMutex);

    if (state.index >= state.hashes.size()) {
      break;
    }

    std::size_t i = state.index;
    state.index++;
    indexUniqueLock.unlock();

    for (std::size_t j = i + 1; j < state.hashes.size(); ++j) {
      if (hashesAreComparable(state.hashes[i], state.hashes[j])) {
        double similarityScore =
            compareHashes(state.hashes[i], state.hashes[j]);

        if (similarityScore >= state.similarityThreshold) {
          const std::lock_guard<std::mutex> coutLockGuard(state.coutMutex);
          std::cout << '"' << state.hashes[i].path << "\" and \""
                    << state.hashes[j].path << "\" are about "
                    << similarityScore << "% similar." << std::endl;
        }
      }
    }
  }
}

// numThreads includes main thread.
void compareHashes(const std::vector<FuzzyHash> &hashes,
                   int similarityThreshold, std::size_t numThreads) {
  if (numThreads <= 1) {
    compareHashes(hashes, similarityThreshold);
    return;
  }

  numThreads--;

  SharedState state(hashes, similarityThreshold);
  std::vector<std::thread> threads(numThreads);

  for (auto &thread : threads) {
    thread =
        std::thread(compareHashAtIndexWithAllSubsequentHashes, std::ref(state));
  }

  compareHashAtIndexWithAllSubsequentHashes(state);

  for (auto &thread : threads) {
    thread.join();
  }
}

constexpr int DEFAULT_SIMILARITY_THRESHOLD = 50;
constexpr int MIN_SIMILARITY_THRESHOLD = 0;
constexpr int MAX_SIMILARITY_THRESHOLD = 99;

constexpr std::size_t DEFAULT_NUM_THREADS = 1;
constexpr std::size_t MIN_NUM_THREADS = 1;
constexpr std::size_t MAX_NUM_THREADS = 256;

const std::unordered_map<std::string, OptionAttributes> validOptions{
    {"--similarity-threshold",
     {true,
      "Display only the file pairs with a similarity score greater than or "
      "equal to this threshold (default: " +
          std::to_string(DEFAULT_SIMILARITY_THRESHOLD) + ")."}},
    {"--num-threads",
     {true, "Number of threads the program will use (default: " +
                std::to_string(DEFAULT_NUM_THREADS) + ")."}}};

int main(int argc, char **argv) {
  try {
    const CommandLineArguments arguments(argc, argv, validOptions);
    if (arguments.arguments.empty()) {
      std::cerr << "Usage: " << arguments.program
                << " [options] <text file with hashes>..." << std::endl;
      arguments.printValidOptions(std::cerr);
      return 1;
    }

    int similarityThreshold = DEFAULT_SIMILARITY_THRESHOLD;
    std::size_t numThreads = DEFAULT_NUM_THREADS;

    if (arguments.options.find("--similarity-threshold") !=
        arguments.options.end()) {
      similarityThreshold = arguments.getOptionValueAsInt(
          "--similarity-threshold", MIN_SIMILARITY_THRESHOLD,
          MAX_SIMILARITY_THRESHOLD);
    }

    if (arguments.options.find("--num-threads") != arguments.options.end()) {
      numThreads = arguments.getOptionValueAsULong(
          "--num-threads", MIN_NUM_THREADS, MAX_NUM_THREADS);
    }

    std::vector<FuzzyHash> hashes;

    std::cout << "Reading hashes." << std::endl;
    readHashes(hashes, arguments.arguments);

    std::cout << "Comparing hashes." << std::endl;
    compareHashes(hashes, similarityThreshold, numThreads);
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;
    return 1;
  }
}

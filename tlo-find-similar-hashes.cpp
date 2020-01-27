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

namespace {
void readHashesFromFile(std::vector<tlo::FuzzyHash> &hashes,
                        const fs::path &file) {
  std::ifstream ifstream(file, std::ifstream::in);
  std::string line;

  while (std::getline(ifstream, line)) {
    try {
      hashes.push_back(tlo::parseHash(line));
    } catch (const std::exception &exception) {
      std::cerr << exception.what() << std::endl;
    }
  }
}

std::vector<tlo::FuzzyHash> readHashes(
    const std::vector<std::string> &arguments) {
  std::vector<tlo::FuzzyHash> hashes;

  for (std::size_t i = 0; i < arguments.size(); ++i) {
    fs::path path = arguments[i];

    if (!fs::is_regular_file(path)) {
      std::cerr << "Error: \"" << path.generic_string() << "\" is not a file."
                << std::endl;
      continue;
    }

    readHashesFromFile(hashes, path);
  }

  return hashes;
}

void compareHashes(const std::vector<tlo::FuzzyHash> &hashes,
                   int similarityThreshold, bool printStatus) {
  std::size_t numHashesDone = 0;

  for (std::size_t i = 0; i < hashes.size(); ++i) {
    for (std::size_t j = i + 1; j < hashes.size(); ++j) {
      if (tlo::hashesAreComparable(hashes[i], hashes[j])) {
        double similarityScore = compareHashes(hashes[i], hashes[j]);

        if (similarityScore >= similarityThreshold) {
          std::cout << '"' << hashes[i].path << "\" and \"" << hashes[j].path
                    << "\" are about " << similarityScore << "% similar."
                    << std::endl;
        }
      }
    }

    if (printStatus) {
      numHashesDone++;

      std::cerr << "Done with " << numHashesDone;

      if (numHashesDone == 1) {
        std::cerr << " hash.";
      } else {
        std::cerr << " hashes.";
      }

      std::cerr << std::endl;
    }
  }
}

struct SharedState {
  std::mutex indexMutex;
  std::size_t index = 0;

  std::mutex outputMutex;
  std::size_t numHashesDone = 0;

  const std::vector<tlo::FuzzyHash> &hashes;
  const int similarityThreshold;
  const bool printStatus;

  SharedState(const std::vector<tlo::FuzzyHash> &hashes_,
              int similarityThreshold_, bool printStatus_)
      : hashes(hashes_),
        similarityThreshold(similarityThreshold_),
        printStatus(printStatus_) {}
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
      if (tlo::hashesAreComparable(state.hashes[i], state.hashes[j])) {
        double similarityScore =
            compareHashes(state.hashes[i], state.hashes[j]);

        if (similarityScore >= state.similarityThreshold) {
          const std::lock_guard<std::mutex> outputLockGuard(state.outputMutex);
          std::cout << '"' << state.hashes[i].path << "\" and \""
                    << state.hashes[j].path << "\" are about "
                    << similarityScore << "% similar." << std::endl;
        }
      }
    }

    if (state.printStatus) {
      const std::lock_guard<std::mutex> outputLockGuard(state.outputMutex);

      state.numHashesDone++;

      std::cerr << "Done with " << state.numHashesDone;

      if (state.numHashesDone == 1) {
        std::cerr << " hash.";
      } else {
        std::cerr << " hashes.";
      }

      std::cerr << std::endl;
    }
  }
}

// numThreads includes main thread.
void compareHashes(const std::vector<tlo::FuzzyHash> &hashes,
                   int similarityThreshold, std::size_t numThreads,
                   bool printStatus) {
  if (numThreads <= 1) {
    compareHashes(hashes, similarityThreshold, printStatus);
    return;
  }

  SharedState state(hashes, similarityThreshold, printStatus);
  std::vector<std::thread> threads(numThreads - 1);

  for (auto &thread : threads) {
    thread =
        std::thread(compareHashAtIndexWithAllSubsequentHashes, std::ref(state));
  }

  compareHashAtIndexWithAllSubsequentHashes(state);

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
    const tlo::CommandLineArguments arguments(argc, argv, validOptions);
    if (arguments.arguments().empty()) {
      std::cerr << "Usage: " << arguments.program()
                << " [options] <text file with hashes>..." << std::endl;
      arguments.printValidOptions(std::cerr);
      return 1;
    }

    int similarityThreshold = DEFAULT_SIMILARITY_THRESHOLD;
    std::size_t numThreads = DEFAULT_NUM_THREADS;
    bool printStatus = false;

    if (arguments.specifiedOption("--similarity-threshold")) {
      similarityThreshold = arguments.getOptionValueAsInt(
          "--similarity-threshold", MIN_SIMILARITY_THRESHOLD,
          MAX_SIMILARITY_THRESHOLD);
    }

    if (arguments.specifiedOption("--num-threads")) {
      numThreads = arguments.getOptionValueAsULong(
          "--num-threads", MIN_NUM_THREADS, MAX_NUM_THREADS);
    }

    if (arguments.specifiedOption("--print-status")) {
      printStatus = true;
    }

    if (printStatus) {
      std::cerr << "Reading hashes." << std::endl;
    }

    std::vector<tlo::FuzzyHash> hashes = readHashes(arguments.arguments());

    if (printStatus) {
      std::cerr << "Comparing hashes." << std::endl;
    }

    compareHashes(hashes, similarityThreshold, numThreads, printStatus);
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;
    return 1;
  }
}

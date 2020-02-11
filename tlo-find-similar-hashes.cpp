#include <exception>
#include <iostream>
#include <mutex>
#include <stdexcept>

#include "compare.hpp"
#include "filesystem.hpp"
#include "options.hpp"

namespace {
enum class OutputFormat { REGULAR, CSV, TSV };

void printSimilarPair(const tlo::FuzzyHash &hash1, const tlo::FuzzyHash &hash2,
                      double similarityScore, OutputFormat outputFormat) {
  if (outputFormat == OutputFormat::REGULAR) {
    std::cout << '"' << hash1.path << "\" and \"" << hash2.path
              << "\" are about " << similarityScore << "% similar."
              << std::endl;
  } else if (outputFormat == OutputFormat::CSV) {
    std::cout << '"' << hash1.path << "\",\"" << hash2.path << "\",\""
              << similarityScore << '"' << std::endl;
  } else if (outputFormat == OutputFormat::TSV) {
    std::cout << '"' << hash1.path << "\"\t\"" << hash2.path << "\"\t\""
              << similarityScore << '"' << std::endl;
  }
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

class StatusUpdater : public tlo::HashComparisonEventHandler {
 private:
  const bool printStatus;
  const OutputFormat outputFormat;
  std::size_t numHashesDone = 0;
  std::size_t numSimilarPairs = 0;

 public:
  StatusUpdater(bool printStatus_, OutputFormat outputFormat_)
      : printStatus(printStatus_), outputFormat(outputFormat_) {}

  void onSimilarPairFound(const tlo::FuzzyHash &hash1,
                          const tlo::FuzzyHash &hash2,
                          double similarityScore) override {
    if (printStatus) {
      numSimilarPairs++;
    }

    printSimilarPair(hash1, hash2, similarityScore, outputFormat);
  }

  void onHashDone() override {
    if (printStatus) {
      numHashesDone++;
      ::printStatus(numHashesDone, numSimilarPairs);
    }
  }
};

class SynchronizingStatusUpdater : public tlo::HashComparisonEventHandler {
 private:
  const bool printStatus;
  const OutputFormat outputFormat;
  std::mutex outputMutex;
  std::size_t numHashesDone = 0;
  std::size_t numSimilarPairs = 0;

 public:
  SynchronizingStatusUpdater(bool printStatus_, OutputFormat outputFormat_)
      : printStatus(printStatus_), outputFormat(outputFormat_) {}

  void onSimilarPairFound(const tlo::FuzzyHash &hash1,
                          const tlo::FuzzyHash &hash2,
                          double similarityScore) override {
    const std::lock_guard<std::mutex> outputLockGuard(outputMutex);

    if (printStatus) {
      numSimilarPairs++;
    }

    printSimilarPair(hash1, hash2, similarityScore, outputFormat);
  }

  void onHashDone() override {
    if (printStatus) {
      const std::lock_guard<std::mutex> outputLockGuard(outputMutex);

      numHashesDone++;
      ::printStatus(numHashesDone, numSimilarPairs);
    }
  }
};

// Throws on error.
OutputFormat stringToOutputFormat(const std::string &string) {
  if (string == "regular") {
    return OutputFormat::REGULAR;
  } else if (string == "csv") {
    return OutputFormat::CSV;
  } else if (string == "tsv") {
    return OutputFormat::TSV;
  } else {
    throw std::runtime_error("Error: \"" + string +
                             "\" is not a recognized output format.");
  }
}
}  // namespace

constexpr int DEFAULT_SIMILARITY_THRESHOLD = 50;
constexpr int MIN_SIMILARITY_THRESHOLD = 0;
constexpr int MAX_SIMILARITY_THRESHOLD = 99;

constexpr std::size_t DEFAULT_NUM_THREADS = 1;
constexpr std::size_t MIN_NUM_THREADS = 1;
constexpr std::size_t MAX_NUM_THREADS = 256;

const std::string DEFAULT_OUTPUT_FORMAT = "regular";

const std::map<std::string, tlo::OptionAttributes> VALID_OPTIONS{
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
      "Allow program to print status updates to stderr (default: off)."}},
    {"--output-format",
     {true,
      "Set output format to regular, csv (comma-separated values), or tsv "
      "(tab-separated values) (default: " +
          DEFAULT_OUTPUT_FORMAT + ")."}}};

int main(int argc, char **argv) {
  try {
    const tlo::CommandLine commandLine(argc, argv, VALID_OPTIONS);

    if (commandLine.arguments().empty()) {
      std::cerr << "Usage: " << commandLine.program()
                << " [options] <text file with hashes>...\n"
                << std::endl;
      commandLine.printValidOptions(std::cerr);

      return 1;
    }

    int similarityThreshold = DEFAULT_SIMILARITY_THRESHOLD;
    std::size_t numThreads = DEFAULT_NUM_THREADS;
    bool printStatus = false;
    std::string outputFormat = DEFAULT_OUTPUT_FORMAT;

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

    if (commandLine.specifiedOption("--output-format")) {
      outputFormat = commandLine.getOptionValue("--output-format");
    }

    if (printStatus) {
      std::cerr << "Reading hashes." << std::endl;
    }

    auto paths = tlo::stringsToPaths(commandLine.arguments());
    auto blockSizesToHashes = tlo::readHashes(paths);

    if (printStatus) {
      std::cerr << "Comparing hashes." << std::endl;
    }

    if (numThreads <= 1) {
      StatusUpdater updater(printStatus, stringToOutputFormat(outputFormat));

      tlo::compareHashes(blockSizesToHashes, similarityThreshold, updater,
                         numThreads);
    } else {
      SynchronizingStatusUpdater updater(printStatus,
                                         stringToOutputFormat(outputFormat));

      tlo::compareHashes(blockSizesToHashes, similarityThreshold, updater,
                         numThreads);
    }
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;
    return 1;
  }
}

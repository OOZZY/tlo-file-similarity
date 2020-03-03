#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <tlo-cpp/command-line.hpp>
#include <tlo-cpp/filesystem.hpp>
#include <tlo-cpp/stop.hpp>
#include <tlo-file-similarity/compare.hpp>

namespace {
enum class OutputFormat { REGULAR, CSV, TSV };

constexpr int DEFAULT_SIMILARITY_THRESHOLD = 50;
constexpr int MIN_SIMILARITY_THRESHOLD = 0;
constexpr int MAX_SIMILARITY_THRESHOLD = 99;

constexpr std::size_t DEFAULT_NUM_THREADS = 1;
constexpr std::size_t MIN_NUM_THREADS = 1;
constexpr std::size_t MAX_NUM_THREADS = 256;

constexpr OutputFormat DEFAULT_OUTPUT_FORMAT = OutputFormat::REGULAR;
const std::string DEFAULT_OUTPUT_FORMAT_STRING = "regular";

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
      "Output format can be regular, csv (comma-separated values), or tsv "
      "(tab-separated values) (default: " +
          DEFAULT_OUTPUT_FORMAT_STRING + ")."}}};

struct Config {
  int similarityThreshold = DEFAULT_SIMILARITY_THRESHOLD;
  std::size_t numThreads = DEFAULT_NUM_THREADS;
  bool printStatus = false;
  OutputFormat outputFormat = DEFAULT_OUTPUT_FORMAT;

  Config(const tlo::CommandLine &commandLine) {
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
      std::string string = commandLine.getOptionValue("--output-format");

      if (string == "regular") {
        outputFormat = OutputFormat::REGULAR;
      } else if (string == "csv") {
        outputFormat = OutputFormat::CSV;
      } else if (string == "tsv") {
        outputFormat = OutputFormat::TSV;
      } else {
        throw std::runtime_error("Error: \"" + string +
                                 "\" is not a recognized output format.");
      }
    }
  }
};

void printSimilarPair(const tfs::FuzzyHash &hash1, const tfs::FuzzyHash &hash2,
                      double similarityScore, OutputFormat outputFormat) {
  if (outputFormat == OutputFormat::REGULAR) {
    std::cout << '"' << hash1.filePath << "\" and \"" << hash2.filePath
              << "\" are about " << similarityScore << "% similar."
              << std::endl;
  } else if (outputFormat == OutputFormat::CSV) {
    std::cout << '"' << hash1.filePath << "\",\"" << hash2.filePath << "\",\""
              << similarityScore << '"' << std::endl;
  } else if (outputFormat == OutputFormat::TSV) {
    std::cout << '"' << hash1.filePath << "\"\t\"" << hash2.filePath << "\"\t\""
              << similarityScore << '"' << std::endl;
  }
}

class AbstractEventHandler : public tfs::HashComparisonEventHandler {
 protected:
  const bool printStatus;
  const OutputFormat outputFormat;

  std::size_t numHashesDone = 0;
  std::size_t numSimilarPairs = 0;

 public:
  AbstractEventHandler(const Config &config)
      : printStatus(config.printStatus), outputFormat(config.outputFormat) {}

  void onSimilarPairFound(const tfs::FuzzyHash &hash1,
                          const tfs::FuzzyHash &hash2,
                          double similarityScore) override {
    if (printStatus) {
      numSimilarPairs++;
    }

    printSimilarPair(hash1, hash2, similarityScore, outputFormat);
  }
};

void printStatus(std::size_t numHashesDone, std::size_t numSimilarPairs) {
  std::cerr << "Done with " << numHashesDone << ' '
            << (numHashesDone == 1 ? "hash" : "hashes") << ". ";
  std::cerr << "Found " << numSimilarPairs << " similar "
            << (numSimilarPairs == 1 ? "pair" : "pairs") << '.' << std::endl;
}

class EventHandler : public AbstractEventHandler {
 public:
  using AbstractEventHandler::AbstractEventHandler;

  void onHashDone() override {
    if (printStatus) {
      numHashesDone++;
      ::printStatus(numHashesDone, numSimilarPairs);
    }
  }
};

class SynchronizingEventHandler : public AbstractEventHandler {
 private:
  std::mutex outputMutex;

 public:
  using AbstractEventHandler::AbstractEventHandler;

  void onSimilarPairFound(const tfs::FuzzyHash &hash1,
                          const tfs::FuzzyHash &hash2,
                          double similarityScore) override {
    const std::lock_guard<std::mutex> outputLockGuard(outputMutex);

    AbstractEventHandler::onSimilarPairFound(hash1, hash2, similarityScore);
  }

  void onHashDone() override {
    if (printStatus) {
      const std::lock_guard<std::mutex> outputLockGuard(outputMutex);

      numHashesDone++;
      ::printStatus(numHashesDone, numSimilarPairs);
    }
  }
};

std::unique_ptr<AbstractEventHandler> makeEventHandler(const Config &config) {
  if (config.numThreads <= 1) {
    return std::make_unique<EventHandler>(config);
  } else {
    return std::make_unique<SynchronizingEventHandler>(config);
  }
}
}  // namespace

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

    tlo::registerInterruptSignalHandler(tloRequestStop);

    const Config config(commandLine);
    const auto paths =
        tlo::stringsToPaths(commandLine.arguments(), tlo::PathType::CANONICAL);

    if (config.printStatus) {
      std::cerr << "Reading hashes." << std::endl;
    }

    auto blockSizesToHashes = tfs::readHashesForComparison(paths);
    std::unique_ptr<AbstractEventHandler> handler = makeEventHandler(config);

    if (config.printStatus) {
      std::cerr << "Comparing hashes." << std::endl;
    }

    tfs::compareHashes(blockSizesToHashes, config.similarityThreshold, *handler,
                       config.numThreads);
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;

    return 1;
  }
}

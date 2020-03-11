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
    {"--verbose",
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
  bool verbose = false;
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

    if (commandLine.specifiedOption("--verbose")) {
      verbose = true;
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

class AbstractEventHandler : public tfs::HashComparisonEventHandler {
 protected:
  const bool verbose;
  const OutputFormat outputFormat;
  const std::size_t numHashesToCompare;

  std::size_t numHashesDone = 0;
  std::size_t numSimilarPairs = 0;

  void printStatus() {
    std::cerr << "Done with " << numHashesDone << ' '
              << (numHashesDone == 1 ? "hash" : "hashes") << " out of "
              << numHashesToCompare << ". ";
    std::cerr << "Found " << numSimilarPairs << " similar "
              << (numSimilarPairs == 1 ? "pair" : "pairs") << '.' << std::endl;
  }

 private:
  void printSimilarPair(const tfs::FuzzyHash &hash1,
                        const tfs::FuzzyHash &hash2, double similarityScore) {
    if (outputFormat == OutputFormat::REGULAR) {
      std::cout << '"' << hash1.filePath << "\" and \"" << hash2.filePath
                << "\" are about " << similarityScore << "% similar."
                << std::endl;
    } else if (outputFormat == OutputFormat::CSV) {
      std::cout << '"' << hash1.filePath << "\",\"" << hash2.filePath << "\",\""
                << similarityScore << '"' << std::endl;
    } else if (outputFormat == OutputFormat::TSV) {
      std::cout << '"' << hash1.filePath << "\"\t\"" << hash2.filePath
                << "\"\t\"" << similarityScore << '"' << std::endl;
    }
  }

 public:
  AbstractEventHandler(const Config &config, std::size_t numHashesToCompare_)
      : verbose(config.verbose),
        outputFormat(config.outputFormat),
        numHashesToCompare(numHashesToCompare_) {}

  void onSimilarPairFound(const tfs::FuzzyHash &hash1,
                          const tfs::FuzzyHash &hash2,
                          double similarityScore) override {
    if (verbose) {
      numSimilarPairs++;
    }

    printSimilarPair(hash1, hash2, similarityScore);
  }
};

class EventHandler : public AbstractEventHandler {
 public:
  using AbstractEventHandler::AbstractEventHandler;

  void onHashDone() override {
    if (verbose) {
      numHashesDone++;
      printStatus();
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
    if (verbose) {
      const std::lock_guard<std::mutex> outputLockGuard(outputMutex);

      numHashesDone++;
      printStatus();
    }
  }
};

std::unique_ptr<AbstractEventHandler> makeEventHandler(
    const Config &config, std::size_t numHashesToCompare) {
  if (config.numThreads <= 1) {
    return std::make_unique<EventHandler>(config, numHashesToCompare);
  } else {
    return std::make_unique<SynchronizingEventHandler>(config,
                                                       numHashesToCompare);
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

    if (config.verbose) {
      std::cerr << "Reading hashes." << std::endl;
    }

    const auto [blockSizesToHashes, numHashes] =
        tfs::readHashesForComparison(paths);
    std::unique_ptr<AbstractEventHandler> handler =
        makeEventHandler(config, numHashes);

    if (config.verbose) {
      std::cerr << "Comparing hashes." << std::endl;
    }

    tfs::compareHashes(blockSizesToHashes, config.similarityThreshold, *handler,
                       config.numThreads);
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;

    return 1;
  }
}

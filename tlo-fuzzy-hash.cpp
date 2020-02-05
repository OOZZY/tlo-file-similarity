#include <exception>
#include <iostream>
#include <mutex>
#include <thread>

#include "fuzzy.hpp"
#include "options.hpp"

namespace {
void printStatus(std::size_t numFilesHashed) {
  std::cerr << "Hashed " << numFilesHashed;

  if (numFilesHashed == 1) {
    std::cerr << " file.";
  } else {
    std::cerr << " files.";
  }

  std::cerr << std::endl;
}

class StatusUpdater : public tlo::FuzzyHashEventHandler {
 private:
  const bool printStatus;
  std::size_t numFilesHashed = 0;

 public:
  StatusUpdater(bool printStatus_) : printStatus(printStatus_) {}

  void onBlockHash() override {
    if (printStatus) {
      std::cerr << '.';
    }
  }

  void onFileHash(const tlo::FuzzyHash &hash) override {
    if (printStatus) {
      std::cerr << std::endl;
    }

    std::cout << hash << std::endl;

    if (printStatus) {
      numFilesHashed++;
      ::printStatus(numFilesHashed);
    }
  }
};

class SynchronizingStatusUpdater : public tlo::FuzzyHashEventHandler {
 private:
  const bool printStatus;
  std::mutex outputMutex;
  std::size_t numFilesHashed = 0;
  std::thread::id previousOutputtingThread;
  bool previousOutputEndsWithNewline = true;

 public:
  SynchronizingStatusUpdater(bool printStatus_) : printStatus(printStatus_) {}

  void onBlockHash() override {
    if (printStatus) {
      const std::lock_guard<std::mutex> outputLockGuard(outputMutex);

      if (previousOutputtingThread == std::this_thread::get_id() &&
          !previousOutputEndsWithNewline) {
        std::cerr << '.';
      } else {
        if (!previousOutputEndsWithNewline) {
          std::cerr << ' ';
        }

        std::cerr << 'T' << std::this_thread::get_id() << '.';
      }

      previousOutputtingThread = std::this_thread::get_id();
      previousOutputEndsWithNewline = false;
    }
  }

  void onFileHash(const tlo::FuzzyHash &hash) override {
    const std::lock_guard<std::mutex> outputLockGuard(outputMutex);

    if (!previousOutputEndsWithNewline) {
      std::cerr << std::endl;
    }

    std::cout << hash << std::endl;

    if (printStatus) {
      numFilesHashed++;
      ::printStatus(numFilesHashed);
    }

    previousOutputtingThread = std::this_thread::get_id();
    previousOutputEndsWithNewline = true;
  }
};
}  // namespace

constexpr std::size_t DEFAULT_NUM_THREADS = 1;
constexpr std::size_t MIN_NUM_THREADS = 1;
constexpr std::size_t MAX_NUM_THREADS = 256;

const std::map<std::string, tlo::OptionAttributes> VALID_OPTIONS{
    {"--num-threads",
     {true, "Number of threads the program will use (default: " +
                std::to_string(DEFAULT_NUM_THREADS) + ")."}},
    {"--print-status",
     {false,
      "Allow program to print status updates to stderr (default: off)."}}};

int main(int argc, char **argv) {
  try {
    const tlo::CommandLine commandLine(argc, argv, VALID_OPTIONS);

    if (commandLine.arguments().empty()) {
      std::cerr << "Usage: " << commandLine.program()
                << " [options] <file or directory>...\n"
                << std::endl;
      commandLine.printValidOptions(std::cerr);

      return 1;
    }

    std::size_t numThreads = DEFAULT_NUM_THREADS;
    bool printStatus = false;

    if (commandLine.specifiedOption("--num-threads")) {
      numThreads = commandLine.getOptionValueAsULong(
          "--num-threads", MIN_NUM_THREADS, MAX_NUM_THREADS);
    }

    if (commandLine.specifiedOption("--print-status")) {
      printStatus = true;
    }

    if (printStatus) {
      std::cerr << "Hashing files." << std::endl;
    }

    if (numThreads <= 1) {
      StatusUpdater updater(printStatus);

      tlo::fuzzyHash(commandLine.arguments(), updater, numThreads);
    } else {
      SynchronizingStatusUpdater updater(printStatus);

      tlo::fuzzyHash(commandLine.arguments(), updater, numThreads);
    }
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;
    return 1;
  }
}

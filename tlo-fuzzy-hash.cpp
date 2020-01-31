#include <condition_variable>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>

#include "fuzzy.hpp"
#include "options.hpp"

namespace fs = std::filesystem;

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
 public:
  const bool printStatus;
  std::size_t numFilesHashed = 0;

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

void hashFiles(const std::vector<std::string> &arguments, bool printStatus) {
  for (std::size_t i = 0; i < arguments.size(); ++i) {
    fs::path path = arguments[i];

    if (!fs::is_regular_file(path) && !fs::is_directory(path)) {
      throw std::runtime_error("Error: \"" + path.generic_string() +
                               "\" is not a file or directory.");
    }
  }

  StatusUpdater updater(printStatus);

  for (std::size_t i = 0; i < arguments.size(); ++i) {
    fs::path path = arguments[i];

    if (fs::is_regular_file(path)) {
      tlo::fuzzyHash(path, &updater);
    } else if (fs::is_directory(path)) {
      for (auto &entry : fs::recursive_directory_iterator(path)) {
        if (fs::is_regular_file(entry.path())) {
          tlo::fuzzyHash(entry.path(), &updater);
        }
      }
    }
  }
}

struct SharedState {
  std::mutex queueMutex;
  bool allFilesQueued = false;
  std::queue<fs::path> files;
  std::condition_variable filesQueued;

  std::mutex outputMutex;
  std::size_t numFilesHashed = 0;
  std::thread::id previousOutputtingThread;
  bool previousOutputEndsWithNewline = true;

  const bool printStatus;

  SharedState(bool printStatus_) : printStatus(printStatus_) {}
};

class SynchronizingStatusUpdater : public tlo::FuzzyHashEventHandler {
 public:
  SharedState &state;

  SynchronizingStatusUpdater(SharedState &state_) : state(state_) {}

  void onBlockHash() override {
    if (state.printStatus) {
      const std::lock_guard<std::mutex> outputLockGuard(state.outputMutex);

      if (state.previousOutputtingThread == std::this_thread::get_id() &&
          !state.previousOutputEndsWithNewline) {
        std::cerr << '.';
      } else {
        if (!state.previousOutputEndsWithNewline) {
          std::cerr << ' ';
        }

        std::cerr << 'T' << std::this_thread::get_id() << '.';
      }

      state.previousOutputtingThread = std::this_thread::get_id();
      state.previousOutputEndsWithNewline = false;
    }
  }

  void onFileHash(const tlo::FuzzyHash &hash) override {
    const std::lock_guard<std::mutex> outputLockGuard(state.outputMutex);

    if (!state.previousOutputEndsWithNewline) {
      std::cerr << std::endl;
    }

    std::cout << hash << std::endl;

    if (state.printStatus) {
      state.numFilesHashed++;
      printStatus(state.numFilesHashed);
    }

    state.previousOutputtingThread = std::this_thread::get_id();
    state.previousOutputEndsWithNewline = true;
  }
};

void hashFilesInQueue(SharedState &state, SynchronizingStatusUpdater &updater) {
  for (;;) {
    std::unique_lock<std::mutex> queueUniqueLock(state.queueMutex);

    if (state.allFilesQueued && state.files.empty()) {
      break;
    }

    while (state.files.empty()) {
      state.filesQueued.wait(queueUniqueLock);
    }

    fs::path file = std::move(state.files.front());
    state.files.pop();

    queueUniqueLock.unlock();

    tlo::fuzzyHash(file, &updater);
  }
}

// numThreads includes main thread.
void hashFiles(const std::vector<std::string> &arguments,
               std::size_t numThreads, bool printStatus) {
  if (numThreads <= 1) {
    hashFiles(arguments, printStatus);
    return;
  }

  for (std::size_t i = 0; i < arguments.size(); ++i) {
    fs::path path = arguments[i];

    if (!fs::is_regular_file(path) && !fs::is_directory(path)) {
      throw std::runtime_error("Error: \"" + path.generic_string() +
                               "\" is not a file or directory.");
    }
  }

  SharedState state(printStatus);
  SynchronizingStatusUpdater updater(state);
  std::vector<std::thread> threads(numThreads - 1);

  for (auto &thread : threads) {
    thread = std::thread(hashFilesInQueue, std::ref(state), std::ref(updater));
  }

  for (std::size_t i = 0; i < arguments.size(); ++i) {
    fs::path path = arguments[i];

    if (fs::is_regular_file(path)) {
      const std::lock_guard<std::mutex> queueLockGuard(state.queueMutex);

      state.files.push(std::move(path));
      state.filesQueued.notify_one();
    } else if (fs::is_directory(path)) {
      for (auto &entry : fs::recursive_directory_iterator(path)) {
        if (fs::is_regular_file(entry.path())) {
          const std::lock_guard<std::mutex> queueLockGuard(state.queueMutex);

          state.files.push(entry.path());
          state.filesQueued.notify_one();
        }
      }
    }
  }

  std::unique_lock<std::mutex> queueUniqueLock(state.queueMutex);
  state.allFilesQueued = true;
  queueUniqueLock.unlock();

  hashFilesInQueue(state, updater);

  for (auto &thread : threads) {
    thread.join();
  }
}
}  // namespace

constexpr std::size_t DEFAULT_NUM_THREADS = 1;
constexpr std::size_t MIN_NUM_THREADS = 1;
constexpr std::size_t MAX_NUM_THREADS = 256;

const std::unordered_map<std::string, tlo::OptionAttributes> VALID_OPTIONS{
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
                << " [options] <file or directory>..." << std::endl;
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

    hashFiles(commandLine.arguments(), numThreads, printStatus);
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;
    return 1;
  }
}

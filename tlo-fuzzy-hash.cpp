#include <condition_variable>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

#include "fuzzy.hpp"
#include "options.hpp"

namespace fs = std::filesystem;

void hashFiles(const std::vector<std::string> &arguments) {
  for (std::size_t i = 0; i < arguments.size(); ++i) {
    fs::path path = arguments[i];

    if (fs::is_regular_file(path)) {
      std::cout << fuzzyHash(path) << std::endl;
    } else if (fs::is_directory(path)) {
      for (auto &entry : fs::recursive_directory_iterator(path)) {
        if (fs::is_regular_file(entry.path())) {
          std::cout << fuzzyHash(entry.path()) << std::endl;
        }
      }
    } else {
      std::cerr << "Error: \"" << path.generic_string()
                << "\" is not a file or directory." << std::endl;
    }
  }
}

struct SharedState {
  std::mutex queueMutex;
  bool allFilesQueued = false;
  std::queue<fs::path> files;
  std::condition_variable filesQueued;

  std::mutex coutMutex;
};

void hashFilesInQueue(SharedState &state) {
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

    FuzzyHash hash = fuzzyHash(file);

    const std::lock_guard<std::mutex> coutLockGuard(state.coutMutex);
    std::cout << hash << std::endl;
  }
}

// numThreads includes main thread.
void hashFiles(const std::vector<std::string> &arguments,
               std::size_t numThreads) {
  if (numThreads <= 1) {
    hashFiles(arguments);
    return;
  }

  numThreads--;

  SharedState state;
  std::vector<std::thread> threads(numThreads);

  for (auto &thread : threads) {
    thread = std::thread(hashFilesInQueue, std::ref(state));
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
    } else {
      std::cerr << "Error: \"" << path.generic_string()
                << "\" is not a file or directory." << std::endl;
    }
  }

  std::unique_lock<std::mutex> queueUniqueLock(state.queueMutex);
  state.allFilesQueued = true;
  queueUniqueLock.unlock();

  hashFilesInQueue(state);

  for (auto &thread : threads) {
    thread.join();
  }
}

constexpr std::size_t DEFAULT_NUM_THREADS = 1;
constexpr std::size_t MIN_NUM_THREADS = 1;
constexpr std::size_t MAX_NUM_THREADS = 256;

const std::unordered_map<std::string, OptionAttributes> validOptions{
    {"--num-threads",
     {true, "Number of threads the program will use (default: " +
                std::to_string(DEFAULT_NUM_THREADS) + ")."}}};

int main(int argc, char **argv) {
  try {
    const CommandLineArguments arguments(argc, argv, validOptions);
    if (arguments.arguments.empty()) {
      std::cerr << "Usage: " << arguments.program
                << " [options] <file or directory>..." << std::endl;
      arguments.printValidOptions(std::cout);
      return 1;
    }

    std::size_t numThreads = DEFAULT_NUM_THREADS;

    if (arguments.options.find("--num-threads") != arguments.options.end()) {
      numThreads = arguments.getOptionValueAsULong(
          "--num-threads", MIN_NUM_THREADS, MAX_NUM_THREADS);
    }

    hashFiles(arguments.arguments, numThreads);
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;
    return 1;
  }
}

#include "fuzzy.hpp"

#include <cassert>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <fstream>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>

#include "filesystem.hpp"
#include "string.hpp"

namespace fs = std::filesystem;

namespace tlo {
std::ostream &operator<<(std::ostream &os, const FuzzyHash &hash) {
  return os << hash.blockSize << ':' << hash.part1 << ':' << hash.part2 << ','
            << hash.filePath;
}

FuzzyHashEventHandler::~FuzzyHashEventHandler() {}

/*
 * Algorithms from:
 * https://www.dfrws.org/sites/default/files/session-files/paper-identifying_almost_identical_files_using_context_triggered_piecewise_hashing.pdf
 * https://github.com/ssdeep-project/ssdeep/blob/master/fuzzy.c
 * http://www.isthe.com/chongo/tech/comp/fnv/
 */

constexpr std::size_t WINDOW_SIZE = 7;

class RollingHasher {
 private:
  uint32_t x = 0;
  uint32_t y = 0;
  uint32_t z = 0;
  uint32_t c = 0;
  uint32_t window[WINDOW_SIZE] = {0};
  bool bytesWereAdded_ = false;

 public:
  void addByte(unsigned char byte) {
    y -= x;
    y += WINDOW_SIZE * byte;
    x += byte;
    x -= window[c % WINDOW_SIZE];
    window[c % WINDOW_SIZE] = byte;
    c++;
    z <<= 5;
    z ^= byte;
    bytesWereAdded_ = true;
  }

  uint32_t getHash() const { return x + y + z; }

  bool bytesWereAdded() { return bytesWereAdded_; }
};

constexpr uint32_t OFFSET_BASIS = 2166136261U;
constexpr uint32_t FNV_PRIME = 16777619U;

class Fnv1Hasher {
 private:
  uint32_t hash = OFFSET_BASIS;
  bool bytesWereAdded_ = false;

 public:
  void addByte(unsigned char byte) {
    hash = (hash * FNV_PRIME) ^ byte;
    bytesWereAdded_ = true;
  }

  uint32_t getHash() const { return hash; }

  bool bytesWereAdded() { return bytesWereAdded_; }
};

constexpr std::size_t MIN_BLOCK_SIZE = 3;
constexpr std::size_t SPAMSUM_LENGTH = 64;
constexpr std::size_t BUFFER_SIZE = 1000000;
constexpr std::string_view BASE64_ALPHABET =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

namespace {
std::pair<std::string, std::string> fuzzyHash(const fs::path &filePath,
                                              std::size_t blockSize,
                                              FuzzyHashEventHandler *handler) {
  std::ifstream ifstream(filePath, std::ifstream::in | std::ifstream::binary);

  if (!ifstream.is_open()) {
    throw std::runtime_error("Error: Failed to open \"" + filePath.string() +
                             "\".");
  }

  std::vector<char> buffer(BUFFER_SIZE, 0);
  RollingHasher rollingHasher;
  Fnv1Hasher fnv1Hasher1;
  Fnv1Hasher fnv1Hasher2;
  std::string part1;
  std::string part2;

  while (!ifstream.eof()) {
    ifstream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    std::size_t numCharsRead = static_cast<std::size_t>(ifstream.gcount());

    for (std::size_t i = 0; i < numCharsRead; ++i) {
      unsigned char byte = static_cast<unsigned char>(buffer[i]);

      rollingHasher.addByte(byte);
      fnv1Hasher1.addByte(byte);
      fnv1Hasher2.addByte(byte);

      if (rollingHasher.getHash() % blockSize == blockSize - 1) {
        part1 +=
            BASE64_ALPHABET[fnv1Hasher1.getHash() % BASE64_ALPHABET.size()];
        fnv1Hasher1 = Fnv1Hasher();

        if (handler) {
          handler->onBlockHash();
        }
      }

      if (rollingHasher.getHash() % (blockSize * 2) == blockSize * 2 - 1) {
        part2 +=
            BASE64_ALPHABET[fnv1Hasher2.getHash() % BASE64_ALPHABET.size()];
        fnv1Hasher2 = Fnv1Hasher();

        if (handler) {
          handler->onBlockHash();
        }
      }
    }
  }

  if (fnv1Hasher1.bytesWereAdded()) {
    part1 += BASE64_ALPHABET[fnv1Hasher1.getHash() % BASE64_ALPHABET.size()];
    fnv1Hasher1 = Fnv1Hasher();

    if (handler) {
      handler->onBlockHash();
    }
  }

  if (fnv1Hasher2.bytesWereAdded()) {
    part2 += BASE64_ALPHABET[fnv1Hasher2.getHash() % BASE64_ALPHABET.size()];
    fnv1Hasher2 = Fnv1Hasher();

    if (handler) {
      handler->onBlockHash();
    }
  }

  return std::pair(part1, part2);
}
}  // namespace

FuzzyHash fuzzyHash(const fs::path &filePath, FuzzyHashEventHandler *handler) {
  if (!fs::is_regular_file(filePath)) {
    throw std::runtime_error("Error: \"" + filePath.string() +
                             "\" is not a file.");
  }

  auto fileSize = getFileSize(filePath);

  if (fileSize == 0) {
    return {MIN_BLOCK_SIZE, "", "", filePath.string()};
  }

  double exponent = std::floor(std::log2(static_cast<double>(fileSize) /
                                         (SPAMSUM_LENGTH * MIN_BLOCK_SIZE)));
  std::size_t blockSize = static_cast<std::size_t>(
      std::ceil(MIN_BLOCK_SIZE * std::pow(2, exponent)));

  if (blockSize < MIN_BLOCK_SIZE) {
    blockSize = MIN_BLOCK_SIZE;
  }

  std::string part1;
  std::string part2;

  for (;;) {
    std::tie(part1, part2) = fuzzyHash(filePath, blockSize, handler);

    if (part1.size() < SPAMSUM_LENGTH / 2 && blockSize / 2 >= MIN_BLOCK_SIZE) {
      blockSize /= 2;
    } else {
      break;
    }
  }

  FuzzyHash hash{blockSize, part1, part2, filePath.string()};

  if (handler) {
    handler->onFileHash(hash);
  }

  return hash;
}

namespace {
void hashFilesWithSingleThread(const std::vector<fs::path> &paths,
                               FuzzyHashEventHandler &handler) {
  for (const auto &path : paths) {
    if (!fs::is_regular_file(path) && !fs::is_directory(path)) {
      throw std::runtime_error("Error: \"" + path.string() +
                               "\" is not a file or directory.");
    }
  }

  for (const auto &path : paths) {
    if (fs::is_regular_file(path)) {
      if (!handler.shouldHashFile(path)) {
        continue;
      }

      handler.collect(fuzzyHash(path, &handler));
    } else if (fs::is_directory(path)) {
      for (const auto &entry : fs::recursive_directory_iterator(path)) {
        if (fs::is_regular_file(entry.path())) {
          if (!handler.shouldHashFile(entry.path())) {
            continue;
          }

          handler.collect(fuzzyHash(entry.path(), &handler));
        }
      }
    }
  }
}

struct SharedState {
  std::mutex queueMutex;
  bool exceptionThrown = false;
  bool allFilesQueued = false;
  std::queue<fs::path> filePaths;
  std::condition_variable filesQueued;
};

void hashFilesInQueue(SharedState &state, FuzzyHashEventHandler &handler,
                      std::exception_ptr &exception) {
  try {
    for (;;) {
      std::unique_lock<std::mutex> queueUniqueLock(state.queueMutex);

      if (state.exceptionThrown) {
        break;
      }

      if (state.allFilesQueued && state.filePaths.empty()) {
        break;
      }

      while (state.filePaths.empty()) {
        state.filesQueued.wait(queueUniqueLock);
      }

      fs::path filePath = std::move(state.filePaths.front());

      state.filePaths.pop();
      queueUniqueLock.unlock();

      if (!handler.shouldHashFile(filePath)) {
        continue;
      }

      handler.collect(fuzzyHash(filePath, &handler));
    }
  } catch (...) {
    std::lock_guard<std::mutex> queueLockGuard(state.queueMutex);

    state.exceptionThrown = true;
    exception = std::current_exception();
  }
}

void hashFilesWithMultipleThreads(const std::vector<fs::path> &paths,
                                  FuzzyHashEventHandler &handler,
                                  std::size_t numThreads) {
  assert(numThreads > 1);

  for (const auto &path : paths) {
    if (!fs::is_regular_file(path) && !fs::is_directory(path)) {
      throw std::runtime_error("Error: \"" + path.string() +
                               "\" is not a file or directory.");
    }
  }

  SharedState state;
  std::vector<std::exception_ptr> exceptions(numThreads);
  std::vector<std::thread> threads(numThreads - 1);

  for (std::size_t i = 0; i < threads.size(); ++i) {
    threads[i] = std::thread(hashFilesInQueue, std::ref(state),
                             std::ref(handler), std::ref(exceptions[i + 1]));
  }

  for (const auto &path : paths) {
    if (fs::is_regular_file(path)) {
      const std::lock_guard<std::mutex> queueLockGuard(state.queueMutex);

      state.filePaths.push(path);
      state.filesQueued.notify_one();
    } else if (fs::is_directory(path)) {
      for (const auto &entry : fs::recursive_directory_iterator(path)) {
        if (fs::is_regular_file(entry.path())) {
          const std::lock_guard<std::mutex> queueLockGuard(state.queueMutex);

          state.filePaths.push(entry.path());
          state.filesQueued.notify_one();
        }
      }
    }
  }

  std::unique_lock<std::mutex> queueUniqueLock(state.queueMutex);

  state.allFilesQueued = true;
  queueUniqueLock.unlock();

  hashFilesInQueue(state, handler, exceptions[0]);

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

void fuzzyHash(const std::vector<fs::path> &paths,
               FuzzyHashEventHandler &handler, std::size_t numThreads) {
  if (numThreads <= 1) {
    hashFilesWithSingleThread(paths, handler);
  } else {
    hashFilesWithMultipleThreads(paths, handler, numThreads);
  }
}

FuzzyHash parseHash(const std::string &hash) {
  auto commaPosition = hash.find(',');

  if (commaPosition == std::string::npos) {
    throw std::runtime_error("Error: Hash \"" + hash +
                             "\" does not have a comma.");
  }

  std::vector<std::string> colonSplit =
      split(hash.substr(0, commaPosition), ':');

  if (colonSplit.size() != 3) {
    throw std::runtime_error(
        "Error: Hash \"" + hash +
        "\" has the wrong number of sections separated by a colon.");
  }

  std::size_t blockSize;

  try {
    blockSize = std::stoull(colonSplit[0]);
  } catch (const std::exception &) {
    throw std::runtime_error("Error: Hash \"" + hash +
                             "\" has non-integer block size.");
  }

  return {blockSize, std::move(colonSplit[1]), std::move(colonSplit[2]),
          hash.substr(commaPosition + 1)};
}
}  // namespace tlo

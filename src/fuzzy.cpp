#include "tlo-file-similarity/fuzzy.hpp"

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
#include <tlo-cpp/chrono.hpp>
#include <tlo-cpp/filesystem.hpp>
#include <tlo-cpp/hash.hpp>
#include <tlo-cpp/stop.hpp>
#include <tlo-cpp/string.hpp>
#include <tuple>
#include <unordered_set>
#include <utility>

namespace fs = std::filesystem;

namespace tlo {
std::ostream &operator<<(std::ostream &os, const FuzzyHash &hash) {
  return os << hash.blockSize << ':' << hash.part1 << ':' << hash.part2 << ','
            << hash.filePath;
}

bool operator==(const FuzzyHash &hash1, const FuzzyHash &hash2) {
  return hash1.blockSize == hash2.blockSize && hash1.part1 == hash2.part1 &&
         hash1.part2 == hash2.part2 && hash1.filePath == hash2.filePath;
}

std::size_t HashFuzzyHash::operator()(const FuzzyHash &hash) const {
  BoostStyleHashCombiner combiner;

  return combiner.combineWith(std::hash<std::size_t>()(hash.blockSize))
      .combineWith(std::hash<std::string>()(hash.part1))
      .combineWith(std::hash<std::string>()(hash.part2))
      .combineWith(std::hash<std::string>()(hash.filePath))
      .getHash();
}

std::size_t HashFuzzyHashPath::operator()(const FuzzyHash &hash) const {
  return std::hash<std::string>()(hash.filePath);
}

bool EqualFuzzyHashPath::operator()(const FuzzyHash &hash1,
                                    const FuzzyHash &hash2) const {
  return hash1.filePath == hash2.filePath;
}

FuzzyHashEventHandler::~FuzzyHashEventHandler() = default;

/*
 * Fuzzy hash algorithm, rolling hash algorithm, and SPAMSUM_LENGTH constant
 * from the paper "Identifying Almost Identical Files Using Context Triggered
 * Piecewise Hashing" by Jesse Kornblum (2006). The paper is available at any
 * of the following links:
 * https://www.dfrws.org/sites/default/files/session-files/paper-identifying_almost_identical_files_using_context_triggered_piecewise_hashing.pdf
 * https://doi.org/10.1016/j.diin.2006.06.015
 *
 * Constants WINDOW_SIZE and MIN_BLOCK_SIZE from source file "ssdeep/fuzzy.c"
 * available at: https://github.com/ssdeep-project/ssdeep/blob/master/fuzzy.c
 * (Retrieved January 20, 2020)
 *
 * FNV-1 hash algorithm and constants OFFSET_BASIS and FNV_PRIME from web page
 * "FNV Hash" by Landon Curt Noll available at:
 * http://www.isthe.com/chongo/tech/comp/fnv/ (Retrieved January 20, 2020)
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
std::pair<std::string, std::string> hashUsingBlockSize(
    const fs::path &filePath, std::size_t blockSize,
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

        if (stopRequested.load()) {
          return std::pair(part1, part2);
        }
      }

      if (rollingHasher.getHash() % (blockSize * 2) == blockSize * 2 - 1) {
        part2 +=
            BASE64_ALPHABET[fnv1Hasher2.getHash() % BASE64_ALPHABET.size()];
        fnv1Hasher2 = Fnv1Hasher();

        if (handler) {
          handler->onBlockHash();
        }

        if (stopRequested.load()) {
          return std::pair(part1, part2);
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

FuzzyHash hashFile(const fs::path &filePath, FuzzyHashEventHandler *handler,
                   std::uintmax_t fileSize) {
  if (fileSize == 0) {
    FuzzyHash hash{MIN_BLOCK_SIZE, "", "", filePath.string()};

    if (handler) {
      handler->onBlockHash();
      handler->onFileHash(hash);
    }

    return hash;
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
    std::tie(part1, part2) = hashUsingBlockSize(filePath, blockSize, handler);

    if (stopRequested.load()) {
      part1 += BAD_FUZZY_HASH_CHAR;
      part2 += BAD_FUZZY_HASH_CHAR;
      return {blockSize, part1, part2, filePath.string()};
    }

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
}  // namespace

FuzzyHash fuzzyHash(const fs::path &filePath, FuzzyHashEventHandler *handler) {
  if (!fs::is_regular_file(filePath)) {
    throw std::runtime_error("Error: \"" + filePath.string() +
                             "\" is not a file.");
  }

  return hashFile(filePath, handler, getFileSize(filePath));
}

namespace {
void hashAndCollect(const fs::path &filePath, FuzzyHashEventHandler &handler) {
  std::uintmax_t fileSize = getFileSize(filePath);
  std::string fileLastWriteTime =
      timeToLocalTimestamp(getLastWriteTime(filePath));

  if (handler.shouldHashFile(filePath, fileSize, fileLastWriteTime)) {
    FuzzyHash hash = hashFile(filePath, &handler, fileSize);

    if (stopRequested.load()) {
      return;
    }

    handler.collect(std::move(hash), fileSize, std::move(fileLastWriteTime));
  }
}

void hashFilesWithSingleThread(const std::vector<fs::path> &paths,
                               FuzzyHashEventHandler &handler) {
  for (const auto &path : paths) {
    if (!fs::is_regular_file(path) && !fs::is_directory(path)) {
      throw std::runtime_error("Error: \"" + path.string() +
                               "\" is not a file or directory.");
    }
  }

  std::unordered_set<fs::path, HashPath> pathsSeen;

  for (const auto &path : paths) {
    if (fs::is_regular_file(path)) {
      if (pathsSeen.find(path) == pathsSeen.end()) {
        pathsSeen.insert(path);
        hashAndCollect(path, handler);

        if (stopRequested.load()) {
          return;
        }
      }
    } else if (fs::is_directory(path)) {
      for (const auto &entry : fs::recursive_directory_iterator(path)) {
        if (fs::is_regular_file(entry.path())) {
          if (pathsSeen.find(entry.path()) == pathsSeen.end()) {
            pathsSeen.insert(entry.path());
            hashAndCollect(entry.path(), handler);

            if (stopRequested.load()) {
              return;
            }
          }
        }
      }
    }
  }
}

struct SharedState {
  FuzzyHashEventHandler &handler;

  std::mutex queueMutex;
  bool exceptionThrown = false;
  bool allFilesQueued = false;
  std::queue<fs::path> filePaths;
  std::condition_variable filesQueued;

  SharedState(FuzzyHashEventHandler &handler_) : handler(handler_) {}
};

void hashFilesInQueue(SharedState &state, std::exception_ptr &exception) {
  try {
    for (;;) {
      std::unique_lock<std::mutex> queueUniqueLock(state.queueMutex);

      if (state.exceptionThrown) {
        return;
      }

      if (state.allFilesQueued && state.filePaths.empty()) {
        return;
      }

      while (state.filePaths.empty()) {
        state.filesQueued.wait(queueUniqueLock);

        if (state.exceptionThrown) {
          return;
        }

        if (state.allFilesQueued && state.filePaths.empty()) {
          return;
        }
      }

      fs::path filePath = std::move(state.filePaths.front());

      state.filePaths.pop();
      queueUniqueLock.unlock();

      hashAndCollect(filePath, state.handler);

      if (stopRequested.load()) {
        return;
      }
    }
  } catch (...) {
    std::unique_lock<std::mutex> queueUniqueLock(state.queueMutex);

    state.exceptionThrown = true;
    exception = std::current_exception();
    queueUniqueLock.unlock();
    state.filesQueued.notify_all();
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

  SharedState state(handler);
  std::vector<std::exception_ptr> exceptions(numThreads);
  std::vector<std::thread> threads(numThreads - 1);

  for (std::size_t i = 0; i < threads.size(); ++i) {
    threads[i] = std::thread(hashFilesInQueue, std::ref(state),
                             std::ref(exceptions[i + 1]));
  }

  std::unordered_set<fs::path, HashPath> pathsSeen;

  for (const auto &path : paths) {
    if (fs::is_regular_file(path)) {
      if (pathsSeen.find(path) == pathsSeen.end()) {
        pathsSeen.insert(path);

        std::unique_lock<std::mutex> queueUniqueLock(state.queueMutex);

        state.filePaths.push(path);
        queueUniqueLock.unlock();
        state.filesQueued.notify_one();
      }
    } else if (fs::is_directory(path)) {
      for (const auto &entry : fs::recursive_directory_iterator(path)) {
        if (fs::is_regular_file(entry.path())) {
          if (pathsSeen.find(entry.path()) == pathsSeen.end()) {
            pathsSeen.insert(entry.path());

            std::unique_lock<std::mutex> queueUniqueLock(state.queueMutex);

            state.filePaths.push(entry.path());
            queueUniqueLock.unlock();
            state.filesQueued.notify_one();
          }
        }
      }
    }
  }

  pathsSeen.clear();

  std::unique_lock<std::mutex> queueUniqueLock(state.queueMutex);

  state.allFilesQueued = true;
  queueUniqueLock.unlock();
  state.filesQueued.notify_all();

  hashFilesInQueue(state, exceptions[0]);

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

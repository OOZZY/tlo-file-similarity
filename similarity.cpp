#include "similarity.hpp"

#include <fstream>
#include <functional>
#include <mutex>
#include <numeric>
#include <thread>

#include "filesystem.hpp"
#include "lcs.hpp"

namespace fs = std::filesystem;

class FilePairReader {
 private:
  std::ifstream ifstream1;
  std::ifstream ifstream2;

 public:
  FilePairReader(const fs::path &path1, const fs::path &path2)
      : ifstream1(path1, std::ifstream::in | std::ifstream::binary),
        ifstream2(path2, std::ifstream::in | std::ifstream::binary) {}

  bool hasBlocks() { return !ifstream1.eof() && !ifstream2.eof(); }

  // Returns number of characters read from first file and from second file
  std::pair<std::size_t, std::size_t> readBlocks(std::vector<char> &buffer1,
                                                 std::vector<char> &buffer2) {
    ifstream1.read(buffer1.data(),
                   static_cast<std::streamsize>(buffer1.size()));
    ifstream2.read(buffer2.data(),
                   static_cast<std::streamsize>(buffer2.size()));
    return std::pair<std::size_t, std::size_t>(ifstream1.gcount(),
                                               ifstream2.gcount());
  }
};

double checkFileSimilarity(const fs::path &path1, const fs::path &path2,
                           std::size_t blockSize) {
  if (!fs::is_regular_file(path1) || !fs::is_regular_file(path2)) {
    return 0;
  }

  auto file1Size = getFileSize(path1);
  auto file2Size = getFileSize(path2);

  if (file1Size == 0 || file2Size == 0) {
    return 0;
  }

  FilePairReader reader(path1, path2);
  std::vector<char> buffer1(blockSize, 0);
  std::vector<char> buffer2(blockSize, 0);

  std::size_t lcsLength = 0;

  while (reader.hasBlocks()) {
    auto [size1, size2] = reader.readBlocks(buffer1, buffer2);
    LCSLengthResult result = lcsLength3_(buffer1, 0, size1, buffer2, 0, size2,
                                         lcsLength2_<std::vector<char>>);
    lcsLength += result.lcsLength;
  }

  auto largerFileSize = std::max(file1Size, file2Size);
  return static_cast<double>(lcsLength) / largerFileSize * 100;
}

namespace {
void readAndCompareBlocks(FilePairReader &reader, std::mutex &readerMutex,
                          std::size_t &lcsLength, std::size_t blockSize) {
  std::vector<char> buffer1(blockSize, 0);
  std::vector<char> buffer2(blockSize, 0);

  for (;;) {
    std::unique_lock<std::mutex> lock(readerMutex);

    if (!reader.hasBlocks()) {
      break;
    }

    auto [size1, size2] = reader.readBlocks(buffer1, buffer2);
    lock.unlock();

    LCSLengthResult result = lcsLength3_(buffer1, 0, size1, buffer2, 0, size2,
                                         lcsLength2_<std::vector<char>>);
    lcsLength += result.lcsLength;
  }
}
}  // namespace

double checkFileSimilarity(const fs::path &path1, const fs::path &path2,
                           std::size_t blockSize, std::size_t numThreads) {
  if (numThreads == 1) {
    return checkFileSimilarity(path1, path2, blockSize);
  }

  if (!fs::is_regular_file(path1) || !fs::is_regular_file(path2)) {
    return 0;
  }

  auto file1Size = getFileSize(path1);
  auto file2Size = getFileSize(path2);

  if (file1Size == 0 || file2Size == 0) {
    return 0;
  }

  FilePairReader reader(path1, path2);
  std::mutex readerMutex;
  std::vector<std::thread> threads(numThreads);
  std::vector<std::size_t> lcsLengths(numThreads, 0);

  for (std::size_t i = 0; i < threads.size(); ++i) {
    threads[i] =
        std::thread(readAndCompareBlocks, std::ref(reader),
                    std::ref(readerMutex), std::ref(lcsLengths[i]), blockSize);
  }

  for (std::size_t i = 0; i < threads.size(); ++i) {
    threads[i].join();
  }

  std::size_t lcsLength = std::accumulate(lcsLengths.begin(), lcsLengths.end(),
                                          static_cast<std::size_t>(0));
  auto largerFileSize = std::max(file1Size, file2Size);
  return static_cast<double>(lcsLength) / largerFileSize * 100;
}

#include <algorithm>
#include <iostream>
#include <vector>

#include "filesystem.hpp"
#include "similarity.hpp"

namespace fs = std::filesystem;

constexpr int MINIMUM_SIMILARITY = 90;
constexpr std::size_t BLOCK_SIZE = 100;  // 100 is fastest so far
constexpr std::size_t NUM_THREADS = 1;

void addFilesInDirectory(
    std::vector<std::pair<fs::path, std::uintmax_t>> &filesAndSizes,
    const fs::path &path) {
  if (!fs::is_directory(path)) {
    return;
  }

  for (auto &entry : fs::recursive_directory_iterator(path)) {
    fs::path absolutePath = fs::absolute(entry.path());
    auto fileSize = getFileSize(absolutePath);
    filesAndSizes.push_back(std::make_pair(absolutePath, fileSize));
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <directory>..." << std::endl;
    return 1;
  }

  std::cout << "Building file list." << std::endl;

  std::vector<std::pair<fs::path, std::uintmax_t>> filesAndSizes;

  for (int i = 1; i < argc; ++i) {
    addFilesInDirectory(filesAndSizes, argv[i]);
  }

  std::cout << "Found " << filesAndSizes.size() << " file(s)." << std::endl;

  std::size_t numSimilarPairs = 0;
  std::size_t numPairsCompared = 0;

  for (std::size_t i = 0; i < filesAndSizes.size(); ++i) {
    for (std::size_t j = i + 1; j < filesAndSizes.size(); ++j) {
      std::uintmax_t smallerSize =
          std::min(filesAndSizes[i].second, filesAndSizes[j].second);
      std::uintmax_t largerSize =
          std::max(filesAndSizes[i].second, filesAndSizes[j].second);

      if (static_cast<double>(smallerSize) / largerSize * 100 <
          MINIMUM_SIMILARITY) {
        continue;
      }

      if (filesAndSizes[i].first.extension() !=
          filesAndSizes[j].first.extension()) {
        continue;
      }

      std::cout << "Comparing " << filesAndSizes[i].first << " and "
                << filesAndSizes[j].first << '.' << std::endl;

      double similarityPercentage =
          checkFileSimilarity(filesAndSizes[i].first, filesAndSizes[j].first,
                              BLOCK_SIZE, NUM_THREADS);
      numPairsCompared++;

      if (similarityPercentage >= MINIMUM_SIMILARITY) {
        numSimilarPairs++;
        std::cout << "The files are about " << similarityPercentage
                  << "% similar." << std::endl;
      }
    }
  }

  std::cout << "Compared " << numPairsCompared << " file pair(s)." << std::endl;
  std::cout << "Found " << numSimilarPairs << " similar file pair(s)."
            << std::endl;
}

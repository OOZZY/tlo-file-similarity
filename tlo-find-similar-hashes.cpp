#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "fuzzy.hpp"
#include "string.hpp"

namespace fs = std::filesystem;

constexpr int MIN_SIMILARITY_SCORE = 50;

void readHashes(std::vector<FuzzyHash> &hashes, const fs::path &path) {
  std::ifstream ifstream(path, std::ifstream::in);
  std::string line;

  while (std::getline(ifstream, line)) {
    try {
      hashes.push_back(parseHash(line));
    } catch (const std::exception &exception) {
      std::cerr << exception.what() << std::endl;
    }
  }
}

int main(int argc, char **argv) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: " << argv[0] << " <text file with hashes>..."
                << std::endl;
      return 1;
    }

    std::cout << "Reading hashes." << std::endl;

    std::vector<FuzzyHash> hashes;

    for (int i = 1; i < argc; ++i) {
      fs::path path = argv[i];

      if (!fs::is_regular_file(path)) {
        std::cerr << "Error: \"" << path.generic_string() << "\" is not a file."
                  << std::endl;
        continue;
      }

      readHashes(hashes, path);
    }

    std::cout << "Comparing hashes." << std::endl;

    for (std::size_t i = 0; i < hashes.size(); ++i) {
      for (std::size_t j = i + 1; j < hashes.size(); ++j) {
        if (hashesAreComparable(hashes[i], hashes[j])) {
          double similarityScore = compareHashes(hashes[i], hashes[j]);

          if (similarityScore >= MIN_SIMILARITY_SCORE) {
            std::cout << '"' << hashes[i].path << "\" and \"" << hashes[j].path
                      << "\" are about " << similarityScore << "% similar."
                      << std::endl;
          }
        }
      }
    }
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;
  }
}

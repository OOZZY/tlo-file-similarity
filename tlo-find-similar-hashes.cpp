#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "fuzzy.hpp"
#include "options.hpp"

namespace fs = std::filesystem;

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

constexpr int DEFAULT_SIMILARITY_THRESHOLD = 50;
constexpr int MIN_SIMILARITY_THRESHOLD = 0;
constexpr int MAX_SIMILARITY_THRESHOLD = 99;

const std::unordered_map<std::string, OptionAttributes> validOptions{
    {"--similarity-threshold",
     {true,
      "Display only the file pairs with a similarity score greater than or "
      "equal to this threshold (default: " +
          std::to_string(DEFAULT_SIMILARITY_THRESHOLD) + ")."}}};

int main(int argc, char **argv) {
  try {
    const CommandLineArguments arguments(argc, argv, validOptions);
    if (arguments.arguments.empty()) {
      std::cerr << "Usage: " << arguments.program
                << " [options] <text file with hashes>..." << std::endl;
      arguments.printValidOptions(std::cout);
      return 1;
    }

    int similarityThreshold = DEFAULT_SIMILARITY_THRESHOLD;

    if (arguments.options.find("--similarity-threshold") !=
        arguments.options.end()) {
      similarityThreshold = arguments.getOptionValueAsInt(
          "--similarity-threshold", MIN_SIMILARITY_THRESHOLD,
          MAX_SIMILARITY_THRESHOLD);
    }

    std::cout << "Reading hashes." << std::endl;

    std::vector<FuzzyHash> hashes;

    for (std::size_t i = 0; i < arguments.arguments.size(); ++i) {
      fs::path path = arguments.arguments[i];

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

          if (similarityScore >= similarityThreshold) {
            std::cout << '"' << hashes[i].path << "\" and \"" << hashes[j].path
                      << "\" are about " << similarityScore << "% similar."
                      << std::endl;
          }
        }
      }
    }
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;
    return 1;
  }
}

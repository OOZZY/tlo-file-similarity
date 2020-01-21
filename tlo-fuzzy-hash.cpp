#include <exception>
#include <filesystem>
#include <iostream>

#include "fuzzy.hpp"

namespace fs = std::filesystem;

int main(int argc, char **argv) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: " << argv[0] << " <file or directory>..."
                << std::endl;
      return 1;
    }

    for (int i = 1; i < argc; ++i) {
      fs::path path = argv[i];

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
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;
  }
}

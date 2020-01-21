#include <exception>
#include <filesystem>
#include <iostream>

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

int main(int argc, char **argv) {
  try {
    const CommandLineArguments arguments(argc, argv);
    if (arguments.arguments.empty()) {
      std::cerr << "Usage: " << arguments.program << " <file or directory>..."
                << std::endl;
      return 1;
    }

    hashFiles(arguments.arguments);
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;
    return 1;
  }
}

#include "options.hpp"

#include <iostream>

CommandLineArguments::CommandLineArguments(
    int argc, char **argv,
    const std::unordered_map<std::string, OptionAttributes> &validOptions_)
    : validOptions(validOptions_) {
  program = argv[0];

  for (int i = 1; i < argc; ++i) {
    std::string argument = argv[i];

    if (argument.size() >= 3 && argument[0] == '-' && argument[1] == '-') {
      auto equalPosition = argument.find("=");

      if (equalPosition == std::string::npos) {
        if (validOptions.find(argument) == validOptions.end()) {
          std::cerr << "Error: \"" << argument << "\" is not a valid option."
                    << std::endl;
          continue;
        }

        options[std::move(argument)] = "";
      } else {
        std::string option = argument.substr(0, equalPosition);

        if (validOptions.find(option) == validOptions.end()) {
          std::cerr << "Error: \"" << option << "\" is not a valid option."
                    << std::endl;
          continue;
        }

        auto nextPosition = equalPosition + 1;
        auto value =
            argument.substr(nextPosition, argument.size() - nextPosition);
        options[std::move(option)] = std::move(value);
      }
    } else {
      arguments.push_back(std::move(argument));
    }
  }
}

void CommandLineArguments::printValidOptions(std::ostream &ostream) const {
  if (!validOptions.empty()) {
    ostream << "Valid options:" << std::endl;

    for (const auto &option : validOptions) {
      ostream << option.first;

      if (option.second.valueRequired) {
        ostream << "=value";
      }

      ostream << std::endl;
      ostream << "  " << option.second.description << std::endl;
    }
  }
}

std::ostream &operator<<(std::ostream &ostream,
                         const CommandLineArguments &arguments) {
  ostream << "{" << arguments.program << ", {";

  bool first = true;

  for (const auto &option : arguments.options) {
    if (!first) {
      ostream << ", ";
    }

    if (option.second.empty()) {
      ostream << option.first;
    } else {
      ostream << option.first << "=" << option.second;
    }

    if (first) {
      first = false;
    }
  }

  ostream << "}, {";

  first = true;

  for (const auto &argument : arguments.arguments) {
    if (!first) {
      ostream << ", ";
    }

    ostream << argument;

    if (first) {
      first = false;
    }
  }

  ostream << "}}";
  return ostream;
}

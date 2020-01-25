#include "options.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace tlo {
CommandLineArguments::CommandLineArguments(
    int argc, char **argv,
    const std::unordered_map<std::string, OptionAttributes> &validOptions)
    : validOptions_(validOptions) {
  program_ = argv[0];

  for (int i = 1; i < argc; ++i) {
    std::string argument = argv[i];

    if (argument.size() >= 3 && argument[0] == '-' && argument[1] == '-') {
      auto equalPosition = argument.find("=");

      if (equalPosition == std::string::npos) {
        if (validOptions_.find(argument) == validOptions_.end()) {
          std::cerr << "Error: \"" << argument << "\" is not a valid option."
                    << std::endl;
          continue;
        }

        options_[std::move(argument)] = "";
      } else {
        std::string option = argument.substr(0, equalPosition);

        if (validOptions_.find(option) == validOptions_.end()) {
          std::cerr << "Error: \"" << option << "\" is not a valid option."
                    << std::endl;
          continue;
        }

        auto nextPosition = equalPosition + 1;
        auto value =
            argument.substr(nextPosition, argument.size() - nextPosition);
        options_[std::move(option)] = std::move(value);
      }
    } else {
      arguments_.push_back(std::move(argument));
    }
  }
}

const std::string &CommandLineArguments::program() const { return program_; }

const std::unordered_map<std::string, std::string>
    &CommandLineArguments::options() const {
  return options_;
}

const std::vector<std::string> &CommandLineArguments::arguments() const {
  return arguments_;
}

const std::unordered_map<std::string, OptionAttributes>
    &CommandLineArguments::validOptions() const {
  return validOptions_;
}

void CommandLineArguments::printValidOptions(std::ostream &ostream) const {
  if (!validOptions_.empty()) {
    ostream << "Valid options:" << std::endl;

    for (const auto &option : validOptions_) {
      ostream << option.first;

      if (option.second.valueRequired) {
        ostream << "=value";
      }

      ostream << std::endl;
      ostream << "  " << option.second.description << std::endl;
    }
  }
}

bool CommandLineArguments::specifiedOption(const std::string &option) const {
  return options_.find(option) != options_.end();
}

constexpr int NUMBER_BASE = 10;

template <class Integer>
Integer getOptionsValueAsInteger(
    const CommandLineArguments &arguments, const std::string &option,
    Integer minValue, Integer maxValue,
    Integer (*stringToInteger)(const std::string &, std::size_t *, int)) {
  Integer value;

  try {
    value =
        stringToInteger(arguments.options().at(option), nullptr, NUMBER_BASE);
  } catch (const std::exception &) {
    throw std::runtime_error("Error: Cannot convert " + option + " value \"" +
                             arguments.options().at(option) + "\" to integer.");
  }

  if (value < minValue) {
    throw std::runtime_error(
        "Error: " + option + " value " + std::to_string(value) +
        " is less than minimum value " + std::to_string(minValue) + ".");
  }

  if (value > maxValue) {
    throw std::runtime_error(
        "Error: " + option + " value " + std::to_string(value) +
        " is greater than maximum value " + std::to_string(maxValue) + ".");
  }

  return value;
}

unsigned long CommandLineArguments::getOptionValueAsULong(
    const std::string &option, unsigned long minValue,
    unsigned long maxValue) const {
  return getOptionsValueAsInteger(*this, option, minValue, maxValue,
                                  std::stoul);
}

int CommandLineArguments::getOptionValueAsInt(const std::string &option,
                                              int minValue,
                                              int maxValue) const {
  return getOptionsValueAsInteger(*this, option, minValue, maxValue, std::stoi);
}

std::ostream &operator<<(std::ostream &ostream,
                         const CommandLineArguments &arguments) {
  ostream << "{" << arguments.program() << ", {";

  bool first = true;

  for (const auto &option : arguments.options()) {
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

  for (const auto &argument : arguments.arguments()) {
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
}  // namespace tlo

#ifndef TLOFS_OPTIONS_HPP
#define TLOFS_OPTIONS_HPP

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

struct OptionAttributes {
  bool valueRequired;
  std::string description;
};

struct CommandLineArguments {
  // Name or path of program (argv[0]).
  std::string program;

  // All command line arguments that are options. Maps options to values.
  std::unordered_map<std::string, std::string> options;

  // All command line arguments that are not options.
  std::vector<std::string> arguments;

  // All valid options. Maps options to their attributes.
  std::unordered_map<std::string, OptionAttributes> validOptions;

  CommandLineArguments(int argc, char **argv,
                       const std::unordered_map<std::string, OptionAttributes>
                           &validOptions_ = {});

  void printValidOptions(std::ostream &ostream) const;
};

std::ostream &operator<<(std::ostream &ostream,
                         const CommandLineArguments &arguments);

#endif  // TLOFS_OPTIONS_HPP

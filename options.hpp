#ifndef TLOFS_OPTIONS_HPP
#define TLOFS_OPTIONS_HPP

#include <climits>
#include <map>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace tlo {
struct OptionDetails {
  std::vector<std::string> values;
  int lastIndex;
};

std::ostream &operator<<(std::ostream &ostream, const OptionDetails &details);

struct OptionAttributes {
  bool valueRequired;
  std::string description;
};

class CommandLine {
 private:
  // Name or path of program (argv[0]).
  std::string program_;

  // All command line arguments that are options. Maps options to values.
  std::unordered_map<std::string, OptionDetails> options_;

  // All command line arguments that are not options.
  std::vector<std::string> arguments_;

  // All valid options. Maps options to their attributes.
  std::map<std::string, OptionAttributes> validOptions_;

 public:
  CommandLine(int argc, char **argv,
              const std::map<std::string, OptionAttributes> &validOptions = {});

  const std::string &program() const;
  const std::unordered_map<std::string, OptionDetails> &options() const;
  const std::vector<std::string> &arguments() const;
  const std::map<std::string, OptionAttributes> &validOptions() const;
  void printValidOptions(std::ostream &ostream) const;

  // Returns whether the given option was specified in the command line.
  bool specifiedOption(const std::string &option) const;

  const std::string &getOptionValue(const std::string &option) const;

  // Throws std::runtime_error on error.
  unsigned long getOptionValueAsULong(const std::string &option,
                                      unsigned long minValue = 0,
                                      unsigned long maxValue = ULONG_MAX) const;

  // Throws std::runtime_error on error.
  int getOptionValueAsInt(const std::string &option, int minValue = INT_MIN,
                          int maxValue = INT_MAX) const;

  const std::vector<std::string> &getOptionValues(
      const std::string &option) const;
  int getOptionLastIndex(const std::string &option) const;
};

std::ostream &operator<<(std::ostream &ostream, const CommandLine &arguments);
}  // namespace tlo

#endif  // TLOFS_OPTIONS_HPP

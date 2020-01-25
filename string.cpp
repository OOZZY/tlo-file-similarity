#include "string.hpp"

#include <algorithm>
#include <iterator>

namespace tlo {
std::vector<std::string> split(const std::string &string, char delimiter) {
  std::vector<std::string> strings;
  std::vector<std::size_t> delimiterPositions;

  for (std::size_t i = 0; i < string.size(); ++i) {
    if (string[i] == delimiter) {
      delimiterPositions.push_back(i);
    }
  }

  if (delimiterPositions.empty()) {
    strings.push_back(string);
  } else {
    std::size_t subStrStartPosition;
    std::size_t subStrLength;

    subStrStartPosition = 0;
    subStrLength = delimiterPositions[0] - subStrStartPosition;
    strings.push_back(string.substr(subStrStartPosition, subStrLength));

    for (std::size_t i = 1; i < delimiterPositions.size(); ++i) {
      subStrStartPosition = delimiterPositions[i - 1] + 1;
      subStrLength = delimiterPositions[i] - subStrStartPosition;
      strings.push_back(string.substr(subStrStartPosition, subStrLength));
    }

    subStrStartPosition = delimiterPositions[delimiterPositions.size() - 1] + 1;
    subStrLength = string.size() - subStrStartPosition;
    strings.push_back(string.substr(subStrStartPosition, subStrLength));
  }

  return strings;
}
}  // namespace tlo

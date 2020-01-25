#ifndef TLOFS_STRING_HPP
#define TLOFS_STRING_HPP

#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace tlo {
std::vector<std::string> split(const std::string &string, char delimiter);

template <class T>
std::string toString(const T &object) {
  std::ostringstream oss;
  oss << object;
  return oss.str();
}
}  // namespace tlo

#endif  // TLOFS_STRING_HPP

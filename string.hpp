#ifndef TLOFS_STRING_HPP
#define TLOFS_STRING_HPP

#include <regex>
#include <string>
#include <vector>

std::vector<std::string> split(const std::string &string,
                               const std::regex &delimiter);

std::vector<std::string> split(const std::string &string, char delimiter);

#endif  // TLOFS_STRING_HPP

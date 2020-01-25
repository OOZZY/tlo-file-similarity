#include "lcs.hpp"

namespace tlo {
std::ostream &operator<<(std::ostream &os, const LCSLengthResult &result) {
  return os << '{' << result.lcsLength << ", " << result.lcsEndPosition1 << ", "
            << result.lcsEndPosition2 << '}';
}

bool operator==(const LCSLengthResult &result1,
                const LCSLengthResult &result2) {
  return result1.lcsLength == result2.lcsLength &&
         result1.lcsEndPosition1 == result2.lcsEndPosition1 &&
         result1.lcsEndPosition2 == result2.lcsEndPosition2;
}

std::size_t lookup(const std::vector<std::vector<std::size_t>> &table,
                   std::size_t i, std::size_t j) {
  if (i < table.size() && j < table[i].size()) {
    return table[i][j];
  } else {
    return 0;
  }
}

std::size_t lookup(const std::vector<std::size_t> &array, std::size_t i) {
  if (i < array.size()) {
    return array[i];
  } else {
    return 0;
  }
}
}  // namespace tlo

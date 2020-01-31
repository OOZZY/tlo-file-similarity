#include "compare.hpp"

#include <algorithm>

#include "lcs.hpp"
#include "string.hpp"

namespace tlo {
bool hashesAreComparable(const FuzzyHash &hash1, const FuzzyHash &hash2) {
  if (hash1.blockSize == hash2.blockSize) {
    return true;
  } else if (hash1.blockSize == 2 * hash2.blockSize) {
    return true;
  } else if (2 * hash1.blockSize == hash2.blockSize) {
    return true;
  } else {
    return false;
  }
}

namespace {
double calculateSimilarity(const std::string &string1,
                           const std::string &string2) {
  auto lcsDistance = lcsLength3(string1, string2).lcsDistance;
  auto maxLCSDistance = ::tlo::maxLCSDistance(string1.size(), string2.size());
  return static_cast<double>(maxLCSDistance - lcsDistance) / maxLCSDistance *
         100.0;
}
}  // namespace

double compareHashes(const FuzzyHash &hash1, const FuzzyHash &hash2) {
  if (hash1.blockSize == hash2.blockSize) {
    double part1Similarity = calculateSimilarity(hash1.part1, hash2.part1);
    double part2Similarity = calculateSimilarity(hash1.part2, hash2.part2);
    return std::max(part1Similarity, part2Similarity);
  } else if (hash1.blockSize == 2 * hash2.blockSize) {
    double partSimilarity = calculateSimilarity(hash1.part1, hash2.part2);
    return partSimilarity;
  } else if (2 * hash1.blockSize == hash2.blockSize) {
    double partSimilarity = calculateSimilarity(hash1.part2, hash2.part1);
    return partSimilarity;
  } else {
    throw std::runtime_error("Error: \"" + toString(hash1) + "\" and \"" +
                             toString(hash2) + "\" are not comparable.");
  }
}
}  // namespace tlo

#include "hash.hpp"

namespace tlo {
JavaStyleHashCombiner &JavaStyleHashCombiner::combineWith(
    std::size_t nextHash) {
  hash = 31 * hash + nextHash;
  return *this;
}

std::size_t JavaStyleHashCombiner::getHash() { return hash; }

BoostStyleHashCombiner &BoostStyleHashCombiner::combineWith(
    std::size_t nextHash) {
  hash ^= nextHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  return *this;
}

std::size_t BoostStyleHashCombiner::getHash() { return hash; }
}  // namespace tlo

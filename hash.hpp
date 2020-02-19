#ifndef TLOFS_HASH_HPP
#define TLOFS_HASH_HPP

#include <cstddef>

namespace tlo {
// Uses the generic hash combining algorithm used in Java.
class JavaStyleHashCombiner {
 private:
  std::size_t hash = 1;

 public:
  // Returns *this.
  JavaStyleHashCombiner &combineWith(std::size_t nextHash);

  std::size_t getHash();
};

// Uses the generic hash combining algorithm used in Boost.
class BoostStyleHashCombiner {
 private:
  std::size_t hash = 0;

 public:
  // Returns *this.
  BoostStyleHashCombiner &combineWith(std::size_t nextHash);

  std::size_t getHash();
};
}  // namespace tlo

#endif  // TLOFS_HASH_HPP

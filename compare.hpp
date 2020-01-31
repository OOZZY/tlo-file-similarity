#ifndef TLOFS_COMPARE_HPP
#define TLOFS_COMPARE_HPP

#include "fuzzy.hpp"

namespace tlo {
bool hashesAreComparable(const FuzzyHash &hash1, const FuzzyHash &hash2);

// Returns score from 0 to 100 of how similar the given hashes are. A score
// closer to 100 means the hashes are more similar. Throws std::runtime_error
// if hashes are not comparable.
double compareHashes(const FuzzyHash &hash1, const FuzzyHash &hash2);
}  // namespace tlo

#endif  // TLOFS_COMPARE_HPP

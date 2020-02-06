#ifndef TLOFS_COMPARE_HPP
#define TLOFS_COMPARE_HPP

#include <unordered_map>

#include "fuzzy.hpp"

namespace tlo {
// Returns score from 0 to 100 of how similar the given strings are. A score
// closer to 100 means the hashes are more similar.
double compareWithLcsDistance(const std::string &string1,
                              const std::string &string2);

// Returns score from 0 to 100 of how similar the given strings are. A score
// closer to 100 means the hashes are more similar.
double compareWithLevenshteinDistance(const std::string &string1,
                                      const std::string &string2);

// Returns score from 0 to 100 of how similar the given strings are. A score
// closer to 100 means the hashes are more similar.
double compareWithDamerLevenDistance(const std::string &string1,
                                     const std::string &string2);

bool hashesAreComparable(const FuzzyHash &hash1, const FuzzyHash &hash2);

// Returns score from 0 to 100 of how similar the given hashes are. A score
// closer to 100 means the hashes are more similar. Throws std::runtime_error
// if hashes are not comparable.
double compareHashes(const FuzzyHash &hash1, const FuzzyHash &hash2);

// Expects strings in paths to be paths to files. If a string not a path to a
// file, will throw std::runtime_error. For each file, expects each line of the
// file to have the fuzzy hash format <blockSize>:<part1>:<part2>,<path>.
// Collects the fuzzy hashes into a map where the keys are block sizes and the
// corresponding value for a key is a vector of fuzzy hashes with that key
// block size.
std::unordered_map<std::size_t, std::vector<FuzzyHash>> readHashes(
    const std::vector<std::string> &paths);

class HashComparisonEventHandler {
 public:
  virtual void onSimilarPairFound(const FuzzyHash &hash1,
                                  const FuzzyHash &hash2,
                                  double similarityScore) = 0;
  virtual void onHashDone() = 0;
  virtual ~HashComparisonEventHandler() = 0;
};

// Compares hashes in the the map. Calls handler.onSimilarPairFound() whenever
// a pair of hashes has a similarity score >= similarityThreshold. Calls
// handler.onHashDone() whenever the function is done comparing a hash to
// comparable hashes. If numThreads > 1, make sure that the handler's member
// functions are synchronized.
void compareHashes(const std::unordered_map<std::size_t, std::vector<FuzzyHash>>
                       &blockSizesToHashes,
                   int similarityThreshold, HashComparisonEventHandler &handler,
                   std::size_t numThreads = 1);
}  // namespace tlo

#endif  // TLOFS_COMPARE_HPP

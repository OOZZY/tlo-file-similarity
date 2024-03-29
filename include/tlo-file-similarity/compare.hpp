#ifndef TLO_FS_COMPARE_HPP
#define TLO_FS_COMPARE_HPP

#include <unordered_map>
#include <utility>

#include "tlo-file-similarity/fuzzy.hpp"

namespace tfs {
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

struct FuzzyHashFromFile : public FuzzyHash {
  // Index (within a vector of paths) of the text file this hash came from.
  std::size_t fileIndex = 0;

  explicit FuzzyHashFromFile(FuzzyHash &&hash);
};

struct HashFuzzyHashFromFile {
  std::size_t operator()(const FuzzyHashFromFile &hash) const;
};

using HashComparisonMap =
    std::unordered_map<std::size_t, std::vector<FuzzyHashFromFile>>;

// Expects textFilePaths to be paths to text files. If a path does not refer to
// a file, will throw std::runtime_error. For each file, expects each line of
// the file to have the fuzzy hash format <blockSize>:<part1>:<part2>,<path>.
// Collects the fuzzy hashes into a map where the keys are block sizes and the
// corresponding value for a key is a vector of fuzzy hashes with that key block
// size. Returns the map and also the number of hashes. If recordingSources is
// true, the fileIndex variable of each hash will be set to the index (within
// textFilePaths) of the text file the hash came from. The fileIndex variable
// will be set to 0 otherwise.
std::pair<HashComparisonMap, std::size_t> readHashesForComparison(
    const std::vector<std::filesystem::path> &textFilePaths,
    bool recordingSources = false);

class HashComparisonEventHandler {
 public:
  virtual void onSimilarPairFound(const FuzzyHashFromFile &hash1,
                                  const FuzzyHashFromFile &hash2,
                                  double similarityScore) = 0;
  virtual void onHashDone() = 0;
  virtual ~HashComparisonEventHandler();
};

// Compares hashes in the the map. Calls handler.onSimilarPairFound() whenever
// a pair of hashes has a similarity score >= similarityThreshold. Calls
// handler.onHashDone() whenever the function is done comparing a hash to
// comparable hashes. If numThreads > 1, make sure that the handler's member
// functions are synchronized.
void compareHashes(const HashComparisonMap &blockSizesToHashes,
                   int similarityThreshold, HashComparisonEventHandler &handler,
                   std::size_t numThreads = 1);
}  // namespace tfs

#endif  // TLO_FS_COMPARE_HPP

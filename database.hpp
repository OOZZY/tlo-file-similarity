#ifndef TLOFS_DATABASE_HPP
#define TLOFS_DATABASE_HPP

#include <unordered_set>

#include "fuzzy.hpp"
#include "sqlite3.hpp"

namespace tlo {
struct FuzzyHashRow : public FuzzyHash {
  std::size_t fileSize;
  std::string fileLastWriteTime;

  FuzzyHashRow() = default;

  FuzzyHashRow(const FuzzyHash &hash) : FuzzyHash(hash) {}
  FuzzyHashRow(FuzzyHash &&hash) : FuzzyHash(std::move(hash)) {}

  FuzzyHashRow(std::string &&filePath_) { filePath = std::move(filePath_); }
};

using FuzzyHashRowSet =
    std::unordered_set<FuzzyHashRow, HashFuzzyHashPath, EqualFuzzyHashPath>;

class FuzzyHashDatabase {
 private:
  Sqlite3Connection connection;
  Sqlite3Statement insertFuzzyHash;
  Sqlite3Statement selectFuzzyHashesGlob;
  Sqlite3Statement updateFuzzyHash;

 public:
  void open(const std::filesystem::path &dbFilePath);
  bool isOpen() const;
  void insertHash(const FuzzyHashRow &newHash);
  void insertHashes(const FuzzyHashRowSet &newHashes);

  // Stores found hashes in results.
  void getHashesForFiles(FuzzyHashRowSet &results,
                         const std::vector<std::filesystem::path> &filePaths);

  // Gets hashes whose filePath is under the given directoryPath or a
  // subdirectory of the given directoryPath. Stores found hashes in results.
  void getHashesForDirectory(FuzzyHashRowSet &results,
                             const std::filesystem::path &directoryPath);

  // Calls getHashesForFiles() with all the file paths in paths. Calls
  // getHashesForDirectory() for each directory path in paths.
  void getHashesForPaths(FuzzyHashRowSet &results,
                         const std::vector<std::filesystem::path> &paths);

  void updateHash(const FuzzyHashRow &modifiedHash);
  void updateHashes(const FuzzyHashRowSet &modifiedHashes);
  void deleteHashesForFiles(
      const std::vector<std::filesystem::path> &filePaths);
};
}  // namespace tlo

#endif  // TLOFS_DATABASE_HPP

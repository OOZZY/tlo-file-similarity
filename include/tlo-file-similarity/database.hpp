#ifndef TLOFS_DATABASE_HPP
#define TLOFS_DATABASE_HPP

#include <tlo-cpp/sqlite3.hpp>
#include <unordered_set>

#include "tlo-file-similarity/fuzzy.hpp"

namespace tfs {
struct FuzzyHashRow : public FuzzyHash {
  std::size_t fileSize = 0;
  std::string fileLastWriteTime;

  FuzzyHashRow() = default;
  FuzzyHashRow(const FuzzyHash &hash);
  FuzzyHashRow(FuzzyHash &&hash);
  FuzzyHashRow(std::string &&filePath_);
};

using FuzzyHashRowSet =
    std::unordered_set<FuzzyHashRow, HashFuzzyHashPath, EqualFuzzyHashPath>;

class FuzzyHashDatabase {
 private:
  tlo::Sqlite3Connection connection;
  tlo::Sqlite3Statement insertFuzzyHash;
  tlo::Sqlite3Statement selectFuzzyHashesGlob;
  tlo::Sqlite3Statement updateFuzzyHash;

 public:
  class EventHandler {
   public:
    virtual void onRowInsert() = 0;
    virtual void onRowUpdate() = 0;
    virtual ~EventHandler();
  };

  void open(const std::filesystem::path &dbFilePath);
  bool isOpen() const;

  // If handler is not nullptr, calls handler->onRowInsert().
  void insertHash(const FuzzyHashRow &newHash, EventHandler *handler = nullptr);

  // If handler is not nullptr, calls handler->onRowInsert().
  void insertHashes(const FuzzyHashRowSet &newHashes,
                    EventHandler *handler = nullptr);

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

  // If handler is not nullptr, calls handler->onRowUpdate().
  void updateHash(const FuzzyHashRow &modifiedHash,
                  EventHandler *handler = nullptr);

  // If handler is not nullptr, calls handler->onRowUpdate().
  void updateHashes(const FuzzyHashRowSet &modifiedHashes,
                    EventHandler *handler = nullptr);

  void deleteHashesForFiles(
      const std::vector<std::filesystem::path> &filePaths);
};
}  // namespace tfs

#endif  // TLOFS_DATABASE_HPP

#include "tlo-file-similarity/database.hpp"

#include <tlo-cpp/stop.hpp>
#include <tlo-cpp/string.hpp>

namespace fs = std::filesystem;

namespace tfs {
FuzzyHashRow::FuzzyHashRow(const FuzzyHash &hash) : FuzzyHash(hash) {}

FuzzyHashRow::FuzzyHashRow(FuzzyHash &&hash) : FuzzyHash(std::move(hash)) {}

FuzzyHashRow::FuzzyHashRow(std::string &&filePath_) {
  filePath = std::move(filePath_);
}

FuzzyHashDatabase::EventHandler::~EventHandler() = default;

namespace {
constexpr std::string_view CREATE_TABLE_FUZZY_HASH =
    R"sql(CREATE TABLE IF NOT EXISTS FuzzyHash (
  blockSize INTEGER NOT NULL,
  part1 TEXT NOT NULL,
  part2 TEXT NOT NULL,
  filePath TEXT PRIMARY KEY NOT NULL,
  fileSize INTEGER NOT NULL,
  fileLastWriteTime TEXT NOT NULL
);)sql";

constexpr std::string_view INSERT_FUZZY_HASH =
    "INSERT INTO FuzzyHash VALUES(:blockSize, :part1, :part2, :filePath, "
    ":fileSize, :fileLastWriteTime);";

constexpr std::string_view SELECT_FUZZY_HASHES_IN =
    "SELECT * FROM FuzzyHash WHERE filePath IN (";
constexpr std::string_view SELECT_FUZZY_HASHES_GLOB =
    "SELECT * FROM FuzzyHash WHERE filePath GLOB :filePathPattern;";

constexpr std::string_view UPDATE_FUZZY_HASH =
    "UPDATE FuzzyHash SET blockSize = :blockSize, part1 = :part1, part2 = "
    ":part2, fileSize = :fileSize, fileLastWriteTime = :fileLastWriteTime "
    "WHERE filePath = :filePath;";

constexpr std::string_view DELETE_FUZZY_HASHES_IN =
    "DELETE FROM FuzzyHash WHERE filePath IN (";
}  // namespace

void FuzzyHashDatabase::open(const fs::path &dbFilePath) {
  connection.open(dbFilePath);

  tlo::Sqlite3Statement(connection, CREATE_TABLE_FUZZY_HASH).step();

  insertFuzzyHash.prepare(connection, INSERT_FUZZY_HASH);
  selectFuzzyHashesGlob.prepare(connection, SELECT_FUZZY_HASHES_GLOB);
  updateFuzzyHash.prepare(connection, UPDATE_FUZZY_HASH);
}

bool FuzzyHashDatabase::isOpen() const { return connection.isOpen(); }

void FuzzyHashDatabase::setEventHandler(EventHandler *handler_) {
  handler = handler_;
}

namespace {
void resetClearBindingsAndBindHash(tlo::Sqlite3Statement &statement,
                                   const FuzzyHashRow &hash) {
  statement.reset();
  statement.clearBindings();
  statement.bindInt64(":blockSize", static_cast<sqlite3_int64>(hash.blockSize));
  statement.bindUtf8Text(":part1", hash.part1);
  statement.bindUtf8Text(":part2", hash.part2);
  statement.bindUtf8Text(":filePath", hash.filePath);
  statement.bindInt64(":fileSize", static_cast<sqlite3_int64>(hash.fileSize));
  statement.bindUtf8Text(":fileLastWriteTime", hash.fileLastWriteTime);
}
}  // namespace

void FuzzyHashDatabase::insertHash(const FuzzyHashRow &newHash) {
  resetClearBindingsAndBindHash(insertFuzzyHash, newHash);
  insertFuzzyHash.step();

  if (handler) {
    handler->onRowInsert();
  }
}

void FuzzyHashDatabase::insertHashes(const FuzzyHashRowSet &newHashes) {
  for (const auto &newHash : newHashes) {
    if (tlo::stopRequested.load()) {
      return;
    }

    insertHash(newHash);
  }
}

namespace {
void bindPaths(tlo::Sqlite3Statement &statement,
               const std::vector<fs::path> &filePaths) {
  for (std::size_t i = 0; i < filePaths.size(); ++i) {
    std::string filePath = filePaths[i].u8string();

    statement.bindUtf8Text(static_cast<int>(i + 1), filePath, SQLITE_TRANSIENT);
  }
}

constexpr int BLOCK_SIZE = 0;
constexpr int PART1 = 1;
constexpr int PART2 = 2;
constexpr int FILE_PATH = 3;
constexpr int FILE_SIZE = 4;
constexpr int FILE_LAST_WRITE_TIME = 5;

void getHashes(FuzzyHashRowSet &results,
               tlo::Sqlite3Statement &selectStatement) {
  while (selectStatement.step() != SQLITE_DONE) {
    FuzzyHashRow hash;

    hash.blockSize =
        static_cast<std::size_t>(selectStatement.columnAsInt64(BLOCK_SIZE));
    hash.part1 = selectStatement.columnAsUtf8Text(PART1).data();
    hash.part2 = selectStatement.columnAsUtf8Text(PART2).data();
    hash.filePath = selectStatement.columnAsUtf8Text(FILE_PATH).data();
    hash.fileSize =
        static_cast<std::size_t>(selectStatement.columnAsInt64(FILE_SIZE));
    hash.fileLastWriteTime =
        selectStatement.columnAsUtf8Text(FILE_LAST_WRITE_TIME).data();

    results.insert(std::move(hash));
  }
}
}  // namespace

void FuzzyHashDatabase::getHashesForFiles(
    FuzzyHashRowSet &results, const std::vector<fs::path> &filePaths) {
  std::string sql = SELECT_FUZZY_HASHES_IN.data() +
                    tlo::join(filePaths.size(), "?", ", ") + ");";
  tlo::Sqlite3Statement selectFuzzyHashesIn(connection, sql);

  bindPaths(selectFuzzyHashesIn, filePaths);
  getHashes(results, selectFuzzyHashesIn);
}

void FuzzyHashDatabase::getHashesForDirectory(FuzzyHashRowSet &results,
                                              const fs::path &directoryPath) {
  selectFuzzyHashesGlob.reset();
  selectFuzzyHashesGlob.clearBindings();

  std::string filePathPattern = directoryPath.u8string();

  filePathPattern += '*';
  selectFuzzyHashesGlob.bindUtf8Text(":filePathPattern", filePathPattern);
  getHashes(results, selectFuzzyHashesGlob);
}

void FuzzyHashDatabase::getHashesForPaths(FuzzyHashRowSet &results,
                                          const std::vector<fs::path> &paths) {
  std::vector<fs::path> filePaths;

  for (const auto &path : paths) {
    if (fs::is_regular_file(path)) {
      filePaths.push_back(path);
    } else if (fs::is_directory(path)) {
      getHashesForDirectory(results, path);
    }
  }

  if (!filePaths.empty()) {
    getHashesForFiles(results, filePaths);
  }
}

void FuzzyHashDatabase::updateHash(const FuzzyHashRow &modifiedHash) {
  resetClearBindingsAndBindHash(updateFuzzyHash, modifiedHash);
  updateFuzzyHash.step();

  if (handler) {
    handler->onRowUpdate();
  }
}

void FuzzyHashDatabase::updateHashes(const FuzzyHashRowSet &modifiedHashes) {
  for (const auto &modifiedHash : modifiedHashes) {
    if (tlo::stopRequested.load()) {
      return;
    }

    updateHash(modifiedHash);
  }
}

void FuzzyHashDatabase::deleteHashesForFiles(
    const std::vector<fs::path> &filePaths) {
  std::string sql = DELETE_FUZZY_HASHES_IN.data() +
                    tlo::join(filePaths.size(), "?", ", ") + ");";
  tlo::Sqlite3Statement deleteFuzzyHashesIn(connection, sql);

  bindPaths(deleteFuzzyHashesIn, filePaths);
  deleteFuzzyHashesIn.step();
}
}  // namespace tfs

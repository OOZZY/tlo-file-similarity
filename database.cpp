#include "database.hpp"

#include "string.hpp"

namespace fs = std::filesystem;

namespace tlo {
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

  Sqlite3Statement(connection, CREATE_TABLE_FUZZY_HASH).step();

  insertFuzzyHash.prepare(connection, INSERT_FUZZY_HASH);
  selectFuzzyHashesGlob.prepare(connection, SELECT_FUZZY_HASHES_GLOB);
  updateFuzzyHash.prepare(connection, UPDATE_FUZZY_HASH);
}

bool FuzzyHashDatabase::isOpen() const { return connection.isOpen(); }

namespace {
void resetClearBindingsAndBindHash(Sqlite3Statement &statement,
                                   const FuzzyHashRow &hash) {
  statement.reset();
  statement.clearBindings();

  int parameterIndex;

  parameterIndex = statement.parameterIndex(":blockSize");
  statement.bindInt64(parameterIndex,
                      static_cast<sqlite3_int64>(hash.blockSize));

  parameterIndex = statement.parameterIndex(":part1");
  statement.bindUtf8Text(parameterIndex, hash.part1);

  parameterIndex = statement.parameterIndex(":part2");
  statement.bindUtf8Text(parameterIndex, hash.part2);

  parameterIndex = statement.parameterIndex(":filePath");
  statement.bindUtf8Text(parameterIndex, hash.filePath);

  parameterIndex = statement.parameterIndex(":fileSize");
  statement.bindInt64(parameterIndex,
                      static_cast<sqlite3_int64>(hash.fileSize));

  parameterIndex = statement.parameterIndex(":fileLastWriteTime");
  statement.bindUtf8Text(parameterIndex, hash.fileLastWriteTime);
}
}  // namespace

void FuzzyHashDatabase::insertHash(const FuzzyHashRow &newHash,
                                   EventHandler *handler) {
  resetClearBindingsAndBindHash(insertFuzzyHash, newHash);
  insertFuzzyHash.step();

  if (handler) {
    handler->onRowInsert();
  }
}

void FuzzyHashDatabase::insertHashes(const FuzzyHashRowSet &newHashes,
                                     EventHandler *handler) {
  for (const auto &newHash : newHashes) {
    insertHash(newHash, handler);
  }
}

namespace {
void bindPaths(Sqlite3Statement &statement,
               const std::vector<fs::path> &filePaths) {
  for (std::size_t i = 0; i < filePaths.size(); ++i) {
    std::string filePath = filePaths[i].string();

    statement.bindUtf8Text(static_cast<int>(i + 1), filePath, SQLITE_TRANSIENT);
  }
}

constexpr int BLOCK_SIZE = 0;
constexpr int PART1 = 1;
constexpr int PART2 = 2;
constexpr int FILE_PATH = 3;
constexpr int FILE_SIZE = 4;
constexpr int FILE_LAST_WRITE_TIME = 5;

void getHashes(FuzzyHashRowSet &results, Sqlite3Statement &selectStatement) {
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
  std::string sql =
      SELECT_FUZZY_HASHES_IN.data() + join(filePaths.size(), "?", ", ") + ");";
  Sqlite3Statement selectFuzzyHashesIn(connection, sql);

  bindPaths(selectFuzzyHashesIn, filePaths);
  getHashes(results, selectFuzzyHashesIn);
}

void FuzzyHashDatabase::getHashesForDirectory(FuzzyHashRowSet &results,
                                              const fs::path &directoryPath) {
  selectFuzzyHashesGlob.reset();
  selectFuzzyHashesGlob.clearBindings();

  int parameterIndex = selectFuzzyHashesGlob.parameterIndex(":filePathPattern");
  std::string filePathPattern = directoryPath.string();

  filePathPattern += '*';
  selectFuzzyHashesGlob.bindUtf8Text(parameterIndex, filePathPattern);
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

void FuzzyHashDatabase::updateHash(const FuzzyHashRow &modifiedHash,
                                   EventHandler *handler) {
  resetClearBindingsAndBindHash(updateFuzzyHash, modifiedHash);
  updateFuzzyHash.step();

  if (handler) {
    handler->onRowUpdate();
  }
}

void FuzzyHashDatabase::updateHashes(const FuzzyHashRowSet &modifiedHashes,
                                     EventHandler *handler) {
  for (const auto &modifiedHash : modifiedHashes) {
    updateHash(modifiedHash, handler);
  }
}

void FuzzyHashDatabase::deleteHashesForFiles(
    const std::vector<fs::path> &filePaths) {
  std::string sql =
      DELETE_FUZZY_HASHES_IN.data() + join(filePaths.size(), "?", ", ") + ");";
  Sqlite3Statement deleteFuzzyHashesIn(connection, sql);

  bindPaths(deleteFuzzyHashesIn, filePaths);
  deleteFuzzyHashesIn.step();
}
}  // namespace tlo

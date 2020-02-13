#include "sqlite3.hpp"

#include <cassert>
#include <stdexcept>

namespace fs = std::filesystem;

namespace tlo {
namespace {
void throwIf(bool condition, int rc, const char *messagePrefix) {
  if (condition) {
    std::string errorString = sqlite3_errstr(rc);
    throw std::runtime_error(messagePrefix + errorString + ".");
  }
}
}  // namespace

Sqlite3Statement::Sqlite3Statement() {}

Sqlite3Statement::Sqlite3Statement(const Sqlite3Connection &connection,
                                   const std::string &sql) {
  assert(connection.connection);

  prepare(connection, sql);
}

Sqlite3Statement::~Sqlite3Statement() { sqlite3_finalize(statement); }

void Sqlite3Statement::prepare(const Sqlite3Connection &connection,
                               const std::string &sql) {
  assert(!statement);
  assert(connection.connection);

  int rc =
      sqlite3_prepare_v2(connection.connection, sql.c_str(),
                         static_cast<int>(sql.size() + 1), &statement, nullptr);

  throwIf(rc != SQLITE_OK, rc, "Error: Failed to prepare SQL statement: ");
}

int Sqlite3Statement::step() {
  assert(statement);

  int rc = sqlite3_step(statement);

  throwIf(rc != SQLITE_ROW && rc != SQLITE_DONE, rc,
          "Error: Failed to evaluate SQL statement: ");
  return rc;
}

const void *Sqlite3Statement::columnAsBlob(int columnIndex) {
  assert(statement);
  assert(columnIndex < numColumnsInCurrentRow());

  return sqlite3_column_blob(statement, columnIndex);
}

double Sqlite3Statement::columnAsDouble(int columnIndex) {
  assert(statement);
  assert(columnIndex < numColumnsInCurrentRow());

  return sqlite3_column_double(statement, columnIndex);
}

int Sqlite3Statement::columnAsInt(int columnIndex) {
  assert(statement);
  assert(columnIndex < numColumnsInCurrentRow());

  return sqlite3_column_int(statement, columnIndex);
}

sqlite3_int64 Sqlite3Statement::columnAsInt64(int columnIndex) {
  assert(statement);
  assert(columnIndex < numColumnsInCurrentRow());

  return sqlite3_column_int64(statement, columnIndex);
}

const unsigned char *Sqlite3Statement::columnAsUtf8Text(int columnIndex) {
  assert(statement);
  assert(columnIndex < numColumnsInCurrentRow());

  return sqlite3_column_text(statement, columnIndex);
}

const void *Sqlite3Statement::columnAsUtf16Text(int columnIndex) {
  assert(statement);
  assert(columnIndex < numColumnsInCurrentRow());

  return sqlite3_column_text16(statement, columnIndex);
}

int Sqlite3Statement::numBytesInBlobOrUtf8Text(int columnIndex) {
  assert(statement);
  assert(columnIndex < numColumnsInCurrentRow());

  return sqlite3_column_bytes(statement, columnIndex);
}

int Sqlite3Statement::numBytesInUtf16Text(int columnIndex) {
  assert(statement);
  assert(columnIndex < numColumnsInCurrentRow());

  return sqlite3_column_bytes16(statement, columnIndex);
}

int Sqlite3Statement::columnType(int columnIndex) {
  assert(statement);
  assert(columnIndex < numColumnsInCurrentRow());

  return sqlite3_column_type(statement, columnIndex);
}

int Sqlite3Statement::numColumnsInResultSet() {
  assert(statement);

  return sqlite3_column_count(statement);
}

int Sqlite3Statement::numColumnsInCurrentRow() {
  assert(statement);

  return sqlite3_data_count(statement);
}

void Sqlite3Statement::reset() {
  assert(statement);

  int rc = sqlite3_reset(statement);

  throwIf(rc != SQLITE_OK, rc, "Error: Failed to reset SQL statement: ");
}

void Sqlite3Statement::bindBlob(int parameterIndex, const void *value,
                                int numBytes, void (*destructor)(void *)) {
  assert(statement);
  assert(parameterIndex <= numParameters());

  int rc =
      sqlite3_bind_blob(statement, parameterIndex, value, numBytes, destructor);

  throwIf(rc != SQLITE_OK, rc, "Error: Failed to bind blob to parameter: ");
}

void Sqlite3Statement::bindBlob64(int parameterIndex, const void *value,
                                  sqlite3_uint64 numBytes,
                                  void (*destructor)(void *)) {
  assert(statement);
  assert(parameterIndex <= numParameters());

  int rc = sqlite3_bind_blob64(statement, parameterIndex, value, numBytes,
                               destructor);

  throwIf(rc != SQLITE_OK, rc, "Error: Failed to bind blob to parameter: ");
}

void Sqlite3Statement::bindDouble(int parameterIndex, double value) {
  assert(statement);
  assert(parameterIndex <= numParameters());

  int rc = sqlite3_bind_double(statement, parameterIndex, value);

  throwIf(rc != SQLITE_OK, rc, "Error: Failed to bind double to parameter: ");
}

void Sqlite3Statement::bindInt(int parameterIndex, int value) {
  assert(statement);
  assert(parameterIndex <= numParameters());

  int rc = sqlite3_bind_int(statement, parameterIndex, value);

  throwIf(rc != SQLITE_OK, rc, "Error: Failed to bind int to parameter: ");
}

void Sqlite3Statement::bindInt64(int parameterIndex, sqlite3_int64 value) {
  assert(statement);
  assert(parameterIndex <= numParameters());

  int rc = sqlite3_bind_int64(statement, parameterIndex, value);

  throwIf(rc != SQLITE_OK, rc, "Error: Failed to bind int64 to parameter: ");
}

void Sqlite3Statement::bindNull(int parameterIndex) {
  assert(statement);
  assert(parameterIndex <= numParameters());

  int rc = sqlite3_bind_null(statement, parameterIndex);

  throwIf(rc != SQLITE_OK, rc, "Error: Failed to bind null to parameter: ");
}

void Sqlite3Statement::bindUtf8Text(int parameterIndex, const char *value,
                                    int numBytes, void (*destructor)(void *)) {
  assert(statement);
  assert(parameterIndex <= numParameters());

  int rc =
      sqlite3_bind_text(statement, parameterIndex, value, numBytes, destructor);

  throwIf(rc != SQLITE_OK, rc,
          "Error: Failed to bind UTF-8 text to parameter: ");
}

void Sqlite3Statement::bindUtf16Text(int parameterIndex, const void *value,
                                     int numBytes, void (*destructor)(void *)) {
  assert(statement);
  assert(parameterIndex <= numParameters());

  int rc = sqlite3_bind_text16(statement, parameterIndex, value, numBytes,
                               destructor);

  throwIf(rc != SQLITE_OK, rc,
          "Error: Failed to bind UTF-16 text to parameter: ");
}

void Sqlite3Statement::bindText64(int parameterIndex, unsigned char encoding,
                                  const char *value, sqlite3_uint64 numBytes,
                                  void (*destructor)(void *)) {
  assert(statement);
  assert(parameterIndex <= numParameters());

  int rc = sqlite3_bind_text64(statement, parameterIndex, value, numBytes,
                               destructor, encoding);

  throwIf(rc != SQLITE_OK, rc, "Error: Failed to bind text to parameter: ");
}

void Sqlite3Statement::bindZeroBlob(int parameterIndex, int numBytes) {
  assert(statement);
  assert(parameterIndex <= numParameters());

  int rc = sqlite3_bind_zeroblob(statement, parameterIndex, numBytes);

  throwIf(rc != SQLITE_OK, rc,
          "Error: Failed to bind zero blob to parameter: ");
}

void Sqlite3Statement::bindZeroBlob64(int parameterIndex,
                                      sqlite3_uint64 numBytes) {
  assert(statement);
  assert(parameterIndex <= numParameters());

  int rc = sqlite3_bind_zeroblob64(statement, parameterIndex, numBytes);

  throwIf(rc != SQLITE_OK, rc,
          "Error: Failed to bind zero blob to parameter: ");
}

void Sqlite3Statement::clearBindings() {
  assert(statement);

  int rc = sqlite3_clear_bindings(statement);

  throwIf(rc != SQLITE_OK, rc, "Error: Failed to clear parameter bindings: ");
}

int Sqlite3Statement::numParameters() {
  assert(statement);

  return sqlite3_bind_parameter_count(statement);
}

const char *Sqlite3Statement::parameterName(int parameterIndex) {
  assert(statement);
  assert(parameterIndex <= numParameters());

  return sqlite3_bind_parameter_name(statement, parameterIndex);
}

int Sqlite3Statement::parameterIndex(const std::string &parameterName) {
  assert(statement);

  return sqlite3_bind_parameter_index(statement, parameterName.c_str());
}

Sqlite3Connection::Sqlite3Connection() {}

Sqlite3Connection::Sqlite3Connection(const fs::path &dbFilePath) {
  open(dbFilePath);
}

Sqlite3Connection::~Sqlite3Connection() { sqlite3_close(connection); }

void Sqlite3Connection::open(const std::filesystem::path &dbFilePath) {
  assert(!connection);

  int rc = sqlite3_open(dbFilePath.generic_string().c_str(), &connection);

  throwIf(rc != SQLITE_OK, rc, "Error: Failed to open database connection: ");
}
}  // namespace tlo

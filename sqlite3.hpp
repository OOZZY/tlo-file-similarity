#ifndef TLOFS_SQLITE3_HPP
#define TLOFS_SQLITE3_HPP

#include <sqlite3.h>

#include <cstdlib>
#include <filesystem>

namespace tlo {
class Sqlite3Connection;

class Sqlite3Statement {
 private:
  sqlite3_stmt *statement;
  int rc;

 public:
  Sqlite3Statement(const Sqlite3Connection &connection, const std::string &sql);
  ~Sqlite3Statement();

  // Returns SQLITE_ROW if a new row of data is ready for processing. Returns
  // SQLITE_DONE when the statement has finished executing successfully.
  int step();

  const void *columnAsBlob(int columnIndex);
  double columnAsDouble(int columnIndex);
  int columnAsInt(int columnIndex);
  sqlite3_int64 columnAsInt64(int columnIndex);
  const unsigned char *columnAsUtf8Text(int columnIndex);
  const void *columnAsUtf16Text(int columnIndex);

  int numBytesInBlobOrUtf8Text(int columnIndex);
  int numBytesInUtf16Text(int columnIndex);
  int columnType(int columnIndex);
  int numColumnsInResultSet();
  int numColumnsInCurrentRow();

  void reset();

  void bindBlob(int parameterIndex, const void *value, int numBytes,
                void (*destructor)(void *) = std::free);
  void bindBlob64(int parameterIndex, const void *value,
                  sqlite3_uint64 numBytes,
                  void (*destructor)(void *) = std::free);
  void bindDouble(int parameterIndex, double value);
  void bindInt(int parameterIndex, int value);
  void bindInt64(int parameterIndex, sqlite3_int64 value);
  void bindNull(int parameterIndex);
  void bindUtf8Text(int parameterIndex, const char *value, int numBytes,
                    void (*destructor)(void *) = std::free);
  void bindUtf16Text(int parameterIndex, const void *value, int numBytes,
                     void (*destructor)(void *) = std::free);
  void bindText64(int parameterIndex, unsigned char encoding, const char *value,
                  sqlite3_uint64 numBytes,
                  void (*destructor)(void *) = std::free);
  void bindZeroBlob(int parameterIndex, int numBytes);
  void bindZeroBlob64(int parameterIndex, sqlite3_uint64 numBytes);

  void clearBindings();
  int numParameters();
  const char *parameterName(int parameterIndex);
  int parameterIndex(const std::string &parameterName);
};

class Sqlite3Connection {
 private:
  sqlite3 *connection;

 public:
  Sqlite3Connection(const std::filesystem::path &path);

  // Make sure all prepared statements associated with this database connection
  // are finalized (destructed) before this destructor is called.
  ~Sqlite3Connection();

  friend Sqlite3Statement::Sqlite3Statement(const Sqlite3Connection &,
                                            const std::string &);
};
}  // namespace tlo

#endif  // TLOFS_SQLITE3_HPP

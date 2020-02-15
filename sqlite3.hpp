#ifndef TLOFS_SQLITE3_HPP
#define TLOFS_SQLITE3_HPP

#include <sqlite3.h>

#include <filesystem>
#include <string_view>

namespace tlo {
class Sqlite3Connection;

class Sqlite3Statement {
 private:
  sqlite3_stmt *statement = nullptr;

 public:
  // If default constructed, make sure prepare() is called before calling any
  // of the other functions.
  Sqlite3Statement();

  Sqlite3Statement(const Sqlite3Connection &connection, std::string_view sql);
  ~Sqlite3Statement();

  void prepare(const Sqlite3Connection &connection, std::string_view sql);
  bool isPrepared() const;

  // Returns SQLITE_ROW if a new row of data is ready for processing. Returns
  // SQLITE_DONE when the statement has finished executing successfully. If
  // reusing a prepared statement after step() has returned SQLITE_DONE, make
  // sure to call reset() before calling step() again.
  int step();

  const void *columnAsBlob(int columnIndex);
  double columnAsDouble(int columnIndex);
  int columnAsInt(int columnIndex);
  sqlite3_int64 columnAsInt64(int columnIndex);
  std::string_view columnAsUtf8Text(int columnIndex);
  std::u16string_view columnAsUtf16Text(int columnIndex);

  int numBytesInBlobOrUtf8Text(int columnIndex);
  int numBytesInUtf16Text(int columnIndex);
  int columnType(int columnIndex);
  int numColumnsInResultSet();
  int numColumnsInCurrentRow();

  void reset();

  // If destructor is SQLITE_STATIC, SQLite will not call destructor. If
  // destructor is SQLITE_TRANSIENT, then SQLite will make its own private copy
  // of the data immediately.
  void bindBlob(int parameterIndex, const void *value, int numBytes,
                void (*destructor)(void *) = SQLITE_STATIC);

  // If destructor is SQLITE_STATIC, SQLite will not call destructor. If
  // destructor is SQLITE_TRANSIENT, then SQLite will make its own private copy
  // of the data immediately.
  void bindBlob64(int parameterIndex, const void *value,
                  sqlite3_uint64 numBytes,
                  void (*destructor)(void *) = SQLITE_STATIC);

  void bindDouble(int parameterIndex, double value);
  void bindInt(int parameterIndex, int value);
  void bindInt64(int parameterIndex, sqlite3_int64 value);
  void bindNull(int parameterIndex);

  // If destructor is SQLITE_STATIC, SQLite will not call destructor. If
  // destructor is SQLITE_TRANSIENT, then SQLite will make its own private copy
  // of the data immediately.
  void bindUtf8Text(int parameterIndex, std::string_view value,
                    void (*destructor)(void *) = SQLITE_STATIC);

  // If destructor is SQLITE_STATIC, SQLite will not call destructor. If
  // destructor is SQLITE_TRANSIENT, then SQLite will make its own private copy
  // of the data immediately.
  void bindUtf16Text(int parameterIndex, std::u16string_view value,
                     void (*destructor)(void *) = SQLITE_STATIC);

  // If destructor is SQLITE_STATIC, SQLite will not call destructor. If
  // destructor is SQLITE_TRANSIENT, then SQLite will make its own private copy
  // of the data immediately. numBytes should not include the NULL terminator.
  void bindText64(int parameterIndex, unsigned char encoding, const char *value,
                  sqlite3_uint64 numBytes,
                  void (*destructor)(void *) = SQLITE_STATIC);

  void bindZeroBlob(int parameterIndex, int numBytes);
  void bindZeroBlob64(int parameterIndex, sqlite3_uint64 numBytes);

  void clearBindings();
  int numParameters();
  std::string_view parameterName(int parameterIndex);
  int parameterIndex(std::string_view parameterName);
};

class Sqlite3Connection {
 private:
  sqlite3 *connection = nullptr;

 public:
  // If default constructed, make sure open() is called before associating any
  // prepared statements with this database connection.
  Sqlite3Connection();

  Sqlite3Connection(const std::filesystem::path &dbFilePath);

  // Make sure all prepared statements associated with this database connection
  // are finalized (destructed) before this destructor is called.
  ~Sqlite3Connection();

  void open(const std::filesystem::path &dbFilePath);
  bool isOpen() const;

  friend Sqlite3Statement::Sqlite3Statement(const Sqlite3Connection &,
                                            std::string_view);
  friend void Sqlite3Statement::prepare(const Sqlite3Connection &,
                                        std::string_view);
};
}  // namespace tlo

#endif  // TLOFS_SQLITE3_HPP

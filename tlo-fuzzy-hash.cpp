#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>

#include "chrono.hpp"
#include "database.hpp"
#include "filesystem.hpp"
#include "fuzzy.hpp"
#include "options.hpp"

namespace fs = std::filesystem;

namespace {
constexpr int MAX_SECOND_DIFFERENCE = 1;

class AbstractHashEventHandler : public tlo::FuzzyHashEventHandler {
 protected:
  const bool printStatus;
  const tlo::FuzzyHashRowSet &knownHashes;

  std::size_t numFilesHashed = 0;

  tlo::FuzzyHashRowSet newHashes;
  tlo::FuzzyHashRowSet modifiedHashes;

 public:
  AbstractHashEventHandler(bool printStatus_,
                           const tlo::FuzzyHashRowSet &knownHashes_)
      : printStatus(printStatus_), knownHashes(knownHashes_) {}

  bool shouldHashFile(const fs::path &filePath, std::uintmax_t fileSize,
                      const std::string &fileLastWriteTime) override {
    auto iterator = knownHashes.find(filePath.string());

    if (iterator != knownHashes.end() && iterator->fileSize == fileSize &&
        tlo::equalTimestamps(iterator->fileLastWriteTime, fileLastWriteTime,
                             MAX_SECOND_DIFFERENCE)) {
      onBlockHash();
      onFileHash(*iterator);
      return false;
    }

    return true;
  }

  const tlo::FuzzyHashRowSet &getNewHashes() const { return newHashes; }

  const tlo::FuzzyHashRowSet &getModifiedHashes() const {
    return modifiedHashes;
  }
};

void printStatus(std::size_t numFilesHashed) {
  std::cerr << "Hashed " << numFilesHashed;

  if (numFilesHashed == 1) {
    std::cerr << " file.";
  } else {
    std::cerr << " files.";
  }

  std::cerr << std::endl;
}

class HashEventHandler : public AbstractHashEventHandler {
 public:
  using AbstractHashEventHandler::AbstractHashEventHandler;

  void onBlockHash() override {
    if (printStatus) {
      std::cerr << '.';
    }
  }

  void onFileHash(const tlo::FuzzyHash &hash) override {
    if (printStatus) {
      std::cerr << std::endl;
    }

    std::cout << hash << std::endl;

    if (printStatus) {
      numFilesHashed++;
      ::printStatus(numFilesHashed);
    }
  }

  void collect(tlo::FuzzyHash &&hash, std::uintmax_t fileSize,
               std::string &&fileLastWriteTime) override {
    tlo::FuzzyHashRow row(std::move(hash));

    row.fileSize = fileSize;
    row.fileLastWriteTime = std::move(fileLastWriteTime);

    if (knownHashes.find(row) == knownHashes.end()) {
      newHashes.insert(std::move(row));
    } else {
      modifiedHashes.insert(std::move(row));
    }
  }
};

class SynchronizingHashEventHandler : public AbstractHashEventHandler {
 private:
  std::mutex outputMutex;
  std::thread::id previousOutputtingThread;
  bool previousOutputEndsWithNewline = true;

  std::mutex newHashesMutex;

 public:
  using AbstractHashEventHandler::AbstractHashEventHandler;

  void onBlockHash() override {
    if (printStatus) {
      const std::lock_guard<std::mutex> outputLockGuard(outputMutex);

      if (previousOutputtingThread == std::this_thread::get_id() &&
          !previousOutputEndsWithNewline) {
        std::cerr << '.';
      } else {
        if (!previousOutputEndsWithNewline) {
          std::cerr << ' ';
        }

        std::cerr << 't' << std::this_thread::get_id() << '.';
      }

      previousOutputtingThread = std::this_thread::get_id();
      previousOutputEndsWithNewline = false;
    }
  }

  void onFileHash(const tlo::FuzzyHash &hash) override {
    const std::lock_guard<std::mutex> outputLockGuard(outputMutex);

    if (!previousOutputEndsWithNewline) {
      std::cerr << std::endl;
    }

    std::cout << hash << std::endl;

    if (printStatus) {
      numFilesHashed++;
      ::printStatus(numFilesHashed);
    }

    previousOutputtingThread = std::this_thread::get_id();
    previousOutputEndsWithNewline = true;
  }

  void collect(tlo::FuzzyHash &&hash, std::uintmax_t fileSize,
               std::string &&fileLastWriteTime) override {
    tlo::FuzzyHashRow row(std::move(hash));

    row.fileSize = fileSize;
    row.fileLastWriteTime = std::move(fileLastWriteTime);

    const std::lock_guard<std::mutex> newHashesLockGuard(newHashesMutex);

    if (knownHashes.find(row) == knownHashes.end()) {
      newHashes.insert(std::move(row));
    } else {
      modifiedHashes.insert(std::move(row));
    }
  }
};

class DatabaseEventHandler : public tlo::FuzzyHashDatabase::EventHandler {
 private:
  std::size_t numHashesInserted = 0;
  std::size_t numHashesUpdated = 0;

 public:
  void onRowInsert() override {
    numHashesInserted++;

    std::cerr << "Inserted " << numHashesInserted;

    if (numHashesInserted == 1) {
      std::cerr << " hash.";
    } else {
      std::cerr << " hashes.";
    }

    std::cerr << std::endl;
  }

  void onRowUpdate() override {
    numHashesUpdated++;

    std::cerr << "Updated " << numHashesUpdated;

    if (numHashesUpdated == 1) {
      std::cerr << " hash.";
    } else {
      std::cerr << " hashes.";
    }

    std::cerr << std::endl;
  }
};

constexpr std::size_t DEFAULT_NUM_THREADS = 1;
constexpr std::size_t MIN_NUM_THREADS = 1;
constexpr std::size_t MAX_NUM_THREADS = 256;

const std::map<std::string, tlo::OptionAttributes> VALID_OPTIONS{
    {"--num-threads",
     {true, "Number of threads the program will use (default: " +
                std::to_string(DEFAULT_NUM_THREADS) + ")."}},
    {"--print-status",
     {false,
      "Allow program to print status updates to stderr (default: off)."}},
    {"--database",
     {true,
      "Store hashes in and get hashes from the database at the specified path "
      "(default: no database used)."}}};

struct Config {
  std::size_t numThreads = DEFAULT_NUM_THREADS;
  bool printStatus = false;
  std::string database;

  Config(const tlo::CommandLine &commandLine) {
    if (commandLine.specifiedOption("--num-threads")) {
      numThreads = commandLine.getOptionValueAsULong(
          "--num-threads", MIN_NUM_THREADS, MAX_NUM_THREADS);
    }

    if (commandLine.specifiedOption("--print-status")) {
      printStatus = true;
    }

    if (commandLine.specifiedOption("--database")) {
      database = commandLine.getOptionValue("--database");
    }
  }
};

tlo::FuzzyHashDatabase openDatabase(const Config &config) {
  tlo::FuzzyHashDatabase hashDatabase;

  if (!config.database.empty()) {
    if (config.printStatus) {
      std::cerr << "Opening database." << std::endl;
    }

    hashDatabase.open(config.database);
  }

  return hashDatabase;
}

tlo::FuzzyHashRowSet getKnownHashes(const Config &config,
                                    tlo::FuzzyHashDatabase &hashDatabase,
                                    const std::vector<fs::path> &paths) {
  tlo::FuzzyHashRowSet knownHashes;

  if (hashDatabase.isOpen()) {
    if (config.printStatus) {
      std::cerr << "Getting known hashes from database." << std::endl;
    }

    hashDatabase.getHashesForPaths(knownHashes, paths);
  }

  return knownHashes;
}

std::unique_ptr<AbstractHashEventHandler> makeHashEventHandler(
    const Config &config, const tlo::FuzzyHashRowSet &knownHashes) {
  if (config.numThreads <= 1) {
    return std::make_unique<HashEventHandler>(config.printStatus, knownHashes);
  } else {
    return std::make_unique<SynchronizingHashEventHandler>(config.printStatus,
                                                           knownHashes);
  }
}

void updateDatabase(const Config &config, tlo::FuzzyHashDatabase &hashDatabase,
                    const AbstractHashEventHandler &hashEventHandler,
                    DatabaseEventHandler *databaseEventHandler) {
  if (hashDatabase.isOpen()) {
    if (config.printStatus) {
      std::cerr << "Adding new hashes to database." << std::endl;
    }

    hashDatabase.insertHashes(hashEventHandler.getNewHashes(),
                              databaseEventHandler);

    if (config.printStatus) {
      std::cerr << "Updating existing hashes in database." << std::endl;
    }

    hashDatabase.updateHashes(hashEventHandler.getModifiedHashes(),
                              databaseEventHandler);
  }
}
}  // namespace

int main(int argc, char **argv) {
  try {
    const tlo::CommandLine commandLine(argc, argv, VALID_OPTIONS);

    if (commandLine.arguments().empty()) {
      std::cerr << "Usage: " << commandLine.program()
                << " [options] <file or directory>...\n"
                << std::endl;
      commandLine.printValidOptions(std::cerr);

      return 1;
    }

    Config config(commandLine);
    auto paths = tlo::stringsToPaths(commandLine.arguments());
    tlo::FuzzyHashDatabase hashDatabase = openDatabase(config);
    tlo::FuzzyHashRowSet knownHashes =
        getKnownHashes(config, hashDatabase, paths);
    std::unique_ptr<AbstractHashEventHandler> hashEventHandler =
        makeHashEventHandler(config, knownHashes);

    if (config.printStatus) {
      std::cerr << "Hashing files." << std::endl;
    }

    tlo::fuzzyHash(paths, *hashEventHandler, config.numThreads);

    DatabaseEventHandler databaseEventHandler;

    updateDatabase(config, hashDatabase, *hashEventHandler,
                   config.printStatus ? &databaseEventHandler : nullptr);
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;

    return 1;
  }
}

#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <tlo-cpp/chrono.hpp>
#include <tlo-cpp/filesystem.hpp>
#include <tlo-cpp/options.hpp>
#include <tlo-cpp/stop.hpp>
#include <unordered_set>

#include "database.hpp"
#include "fuzzy.hpp"

namespace fs = std::filesystem;

namespace {
void printStatus(std::size_t numFilesHashed) {
  std::cerr << "Hashed " << numFilesHashed << ' '
            << (numFilesHashed == 1 ? "file" : "files") << '.' << std::endl;
}

constexpr int MAX_SECOND_DIFFERENCE = 1;

class DatabaseEventHandler : public tlo::FuzzyHashDatabase::EventHandler {
 private:
  std::size_t numHashesInserted = 0;
  std::size_t numHashesUpdated = 0;

 public:
  void onRowInsert() override {
    numHashesInserted++;

    std::cerr << "Inserted " << numHashesInserted << ' '
              << (numHashesInserted == 1 ? "hash" : "hashes") << '.'
              << std::endl;
  }

  void onRowUpdate() override {
    numHashesUpdated++;

    std::cerr << "Updated " << numHashesUpdated << ' '
              << (numHashesUpdated == 1 ? "hash" : "hashes") << '.'
              << std::endl;
  }
};

class AbstractHashEventHandler : public tlo::FuzzyHashEventHandler {
 protected:
  const bool printStatus;

  tlo::FuzzyHashDatabase hashDatabase;
  tlo::FuzzyHashRowSet knownHashes;
  tlo::FuzzyHashRowSet newHashes;
  tlo::FuzzyHashRowSet modifiedHashes;

  std::size_t numFilesHashed = 0;
  bool previousOutputEndsWithNewline = true;

 public:
  AbstractHashEventHandler(bool printStatus_, const std::string &database,
                           const std::vector<fs::path> &paths)
      : printStatus(printStatus_) {
    if (!database.empty()) {
      if (printStatus) {
        std::cerr << "Opening database." << std::endl;
      }

      hashDatabase.open(database);

      if (printStatus) {
        std::cerr << "Getting known hashes from database." << std::endl;
      }

      hashDatabase.getHashesForPaths(knownHashes, paths);
    }
  }

  void onFileHash(const tlo::FuzzyHash &hash) override {
    if (!previousOutputEndsWithNewline) {
      std::cerr << std::endl;
    }

    std::cout << hash << std::endl;

    if (printStatus) {
      numFilesHashed++;
      ::printStatus(numFilesHashed);
    }

    previousOutputEndsWithNewline = true;
  }

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

  void finishOutput() const {
    if (!previousOutputEndsWithNewline) {
      std::cerr << std::endl;
    }
  }

  void updateDatabase() {
    if (hashDatabase.isOpen()) {
      DatabaseEventHandler databaseEventHandler;

      if (printStatus) {
        std::cerr << "Adding new hashes to database." << std::endl;
      }

      hashDatabase.insertHashes(newHashes,
                                printStatus ? &databaseEventHandler : nullptr);

      if (printStatus) {
        std::cerr << "Updating existing hashes in database." << std::endl;
      }

      hashDatabase.updateHashes(modifiedHashes,
                                printStatus ? &databaseEventHandler : nullptr);
    }
  }
};

class HashEventHandler : public AbstractHashEventHandler {
 public:
  using AbstractHashEventHandler::AbstractHashEventHandler;

  void onBlockHash() override {
    if (printStatus) {
      std::cerr << '.';
      previousOutputEndsWithNewline = false;
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

    AbstractHashEventHandler::onFileHash(hash);
    previousOutputtingThread = std::this_thread::get_id();
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

std::unique_ptr<AbstractHashEventHandler> makeHashEventHandler(
    const Config &config, const std::vector<fs::path> &paths) {
  if (config.numThreads <= 1) {
    return std::make_unique<HashEventHandler>(config.printStatus,
                                              config.database, paths);
  } else {
    return std::make_unique<SynchronizingHashEventHandler>(
        config.printStatus, config.database, paths);
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

    tlo::registerInterruptSignalHandler(tloRequestStop);

    Config config(commandLine);
    auto paths = tlo::stringsToPaths(commandLine.arguments());
    std::unique_ptr<AbstractHashEventHandler> hashEventHandler =
        makeHashEventHandler(config, paths);

    if (config.printStatus) {
      std::cerr << "Hashing files." << std::endl;
    }

    tlo::fuzzyHash(paths, *hashEventHandler, config.numThreads);
    hashEventHandler->finishOutput();
    hashEventHandler->updateDatabase();
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;

    return 1;
  }
}

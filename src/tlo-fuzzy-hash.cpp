#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <tlo-cpp/chrono.hpp>
#include <tlo-cpp/command-line.hpp>
#include <tlo-cpp/filesystem.hpp>
#include <tlo-cpp/stop.hpp>
#include <tlo-file-similarity/database.hpp>
#include <tlo-file-similarity/fuzzy.hpp>
#include <unordered_set>

namespace fs = std::filesystem;

namespace {
constexpr std::size_t DEFAULT_NUM_THREADS = 1;
constexpr std::size_t MIN_NUM_THREADS = 1;
constexpr std::size_t MAX_NUM_THREADS = 256;

const std::map<std::string, tlo::OptionAttributes> VALID_OPTIONS{
    {"--num-threads",
     {true, "Number of threads the program will use (default: " +
                std::to_string(DEFAULT_NUM_THREADS) + ")."}},
    {"--verbose",
     {false,
      "Allow program to print status updates to stderr (default: off)."}},
    {"--database",
     {true,
      "Store hashes in and get hashes from the database at the specified path "
      "(default: no database used)."}}};

struct Config {
  std::size_t numThreads = DEFAULT_NUM_THREADS;
  bool verbose = false;
  std::string database;

  Config(const tlo::CommandLine &commandLine) {
    if (commandLine.specifiedOption("--num-threads")) {
      numThreads = commandLine.getOptionValueAsULong(
          "--num-threads", MIN_NUM_THREADS, MAX_NUM_THREADS);
    }

    if (commandLine.specifiedOption("--verbose")) {
      verbose = true;
    }

    if (commandLine.specifiedOption("--database")) {
      database = commandLine.getOptionValue("--database");
    }
  }
};

constexpr int MAX_SECOND_DIFFERENCE = 1;

class DatabaseEventHandler : public tfs::FuzzyHashDatabase::EventHandler {
 private:
  const std::size_t numHashesToInsert;
  const std::size_t numHashesToUpdate;

  std::size_t numHashesInserted = 0;
  std::size_t numHashesUpdated = 0;

 public:
  DatabaseEventHandler(std::size_t numHashesToInsert_,
                       std::size_t numHashesToUpdate_)
      : numHashesToInsert(numHashesToInsert_),
        numHashesToUpdate(numHashesToUpdate_) {}

  void onRowInsert() override {
    numHashesInserted++;

    std::cerr << "Inserted " << numHashesInserted << ' '
              << (numHashesInserted == 1 ? "hash" : "hashes") << " out of "
              << numHashesToInsert << '.' << std::endl;
  }

  void onRowUpdate() override {
    numHashesUpdated++;

    std::cerr << "Updated " << numHashesUpdated << ' '
              << (numHashesUpdated == 1 ? "hash" : "hashes") << " out of "
              << numHashesToUpdate << '.' << std::endl;
  }
};

class AbstractHashEventHandler : public tfs::FuzzyHashEventHandler {
 protected:
  const bool verbose;
  const std::size_t numFilesToHash;

  tfs::FuzzyHashDatabase hashDatabase;
  tfs::FuzzyHashRowSet knownHashes;
  tfs::FuzzyHashRowSet newHashes;
  tfs::FuzzyHashRowSet modifiedHashes;

  std::size_t numFilesHashed = 0;
  bool previousOutputEndsWithNewline = true;

 private:
  void printStatus() {
    std::cerr << "Hashed " << numFilesHashed << ' '
              << (numFilesHashed == 1 ? "file" : "files") << " out of "
              << numFilesToHash << '.' << std::endl;
  }

 public:
  AbstractHashEventHandler(const Config &config,
                           const std::vector<fs::path> &paths,
                           std::size_t numFilesToHash_)
      : verbose(config.verbose), numFilesToHash(numFilesToHash_) {
    if (!config.database.empty()) {
      if (verbose) {
        std::cerr << "Opening database." << std::endl;
      }

      hashDatabase.open(config.database);

      if (verbose) {
        std::cerr << "Getting known hashes from database." << std::endl;
      }

      hashDatabase.getHashesForPaths(knownHashes, paths);
    }
  }

  void onFileHash(const tfs::FuzzyHash &hash) override {
    if (!previousOutputEndsWithNewline) {
      std::cerr << std::endl;
    }

    std::cout << hash << std::endl;

    if (verbose) {
      numFilesHashed++;
      printStatus();
    }

    previousOutputEndsWithNewline = true;
  }

  bool shouldHashFile(const fs::path &filePath, std::uintmax_t fileSize,
                      const std::string &fileLastWriteTime) override {
    auto iterator = knownHashes.find(tfs::FuzzyHashRow(filePath.u8string()));

    if (iterator != knownHashes.end() && iterator->fileSize == fileSize &&
        tlo::equalLocalTimestamps(iterator->fileLastWriteTime,
                                  fileLastWriteTime, MAX_SECOND_DIFFERENCE)) {
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
      DatabaseEventHandler databaseEventHandler(newHashes.size(),
                                                modifiedHashes.size());

      if (verbose) {
        hashDatabase.setEventHandler(databaseEventHandler);
      }

      if (verbose) {
        std::cerr << "Adding new hashes to database." << std::endl;
      }

      hashDatabase.insertHashes(newHashes);

      if (verbose) {
        std::cerr << "Updating existing hashes in database." << std::endl;
      }

      hashDatabase.updateHashes(modifiedHashes);
    }
  }
};

class HashEventHandler : public AbstractHashEventHandler {
 public:
  using AbstractHashEventHandler::AbstractHashEventHandler;

  void onBlockHash() override {
    if (verbose) {
      std::cerr << '.';
      previousOutputEndsWithNewline = false;
    }
  }

  void collect(tfs::FuzzyHash &&hash, std::uintmax_t fileSize,
               std::string &&fileLastWriteTime) override {
    tfs::FuzzyHashRow row(std::move(hash), fileSize,
                          std::move(fileLastWriteTime));

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
    if (verbose) {
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

  void onFileHash(const tfs::FuzzyHash &hash) override {
    const std::lock_guard<std::mutex> outputLockGuard(outputMutex);

    AbstractHashEventHandler::onFileHash(hash);
    previousOutputtingThread = std::this_thread::get_id();
  }

  void collect(tfs::FuzzyHash &&hash, std::uintmax_t fileSize,
               std::string &&fileLastWriteTime) override {
    tfs::FuzzyHashRow row(std::move(hash), fileSize,
                          std::move(fileLastWriteTime));

    const std::lock_guard<std::mutex> newHashesLockGuard(newHashesMutex);

    if (knownHashes.find(row) == knownHashes.end()) {
      newHashes.insert(std::move(row));
    } else {
      modifiedHashes.insert(std::move(row));
    }
  }
};

std::unique_ptr<AbstractHashEventHandler> makeHashEventHandler(
    const Config &config, const std::vector<fs::path> &paths,
    std::size_t numFilesToHash) {
  if (config.numThreads <= 1) {
    return std::make_unique<HashEventHandler>(config, paths, numFilesToHash);
  } else {
    return std::make_unique<SynchronizingHashEventHandler>(config, paths,
                                                           numFilesToHash);
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

    const Config config(commandLine);
    const auto paths =
        tlo::stringsToPaths(commandLine.arguments(), tlo::PathType::CANONICAL);
    const std::vector<fs::path> filePaths = tlo::buildFileList(paths);
    std::unique_ptr<AbstractHashEventHandler> hashEventHandler =
        makeHashEventHandler(config, paths, filePaths.size());

    if (config.verbose) {
      std::cerr << "Hashing files." << std::endl;
    }

    tfs::fuzzyHash(filePaths, *hashEventHandler, config.numThreads);
    hashEventHandler->finishOutput();
    hashEventHandler->updateDatabase();
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << std::endl;

    return 1;
  }
}

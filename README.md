# tlo-file-similarity

Command-line programs that use fuzzy hashing to compare files for similarity and
find files that are similar to each other.

## Build Requirements

* CMake
* C++17 development environment for which CMake can generate build files
* SQLite 3

## Clone, Build, and Run

Clone into tlo-file-similarity directory.

```
$ git clone --branch develop --recursive git@github.com:OOZZY/tlo-file-similarity.git
```

Build (out of source).

```
$ mkdir build
$ cd build
$ cmake -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug ../tlo-file-similarity
$ make
```

Fuzzy hash all files in the samples directory and all its subdirectories.

```
$ ./tlo-fuzzy-hash ../tlo-file-similarity/samples > hashes.txt
```

Compare hashes to find similar files.

```
$ ./tlo-find-similar-hashes hashes.txt
"../tlo-file-similarity/samples/Removed-1st-Half.txt" and "../tlo-file-similarity/samples/Moved-Some-Lines.txt" are about 52.4064% similar.
"../tlo-file-similarity/samples/Removed-1st-Half.txt" and "../tlo-file-similarity/samples/Moved-Some-Words.txt" are about 59.2593% similar.
"../tlo-file-similarity/samples/Removed-1st-Half.txt" and "../tlo-file-similarity/samples/Original.txt" are about 65.5914% similar.
"../tlo-file-similarity/samples/Removed-1st-Half.txt" and "../tlo-file-similarity/samples/Removed-Some-Lines.txt" are about 55.6962% similar.
"../tlo-file-similarity/samples/Removed-1st-Half.txt" and "../tlo-file-similarity/samples/Swapped-3rd-And-4th-Paragraphs.txt" are about 65.5914% similar.
"../tlo-file-similarity/samples/Removed-2nd-Half.txt" and "../tlo-file-similarity/samples/Moved-Some-Lines.txt" are about 54.2553% similar.
"../tlo-file-similarity/samples/Removed-2nd-Half.txt" and "../tlo-file-similarity/samples/Moved-Some-Words.txt" are about 60% similar.
"../tlo-file-similarity/samples/Removed-2nd-Half.txt" and "../tlo-file-similarity/samples/Original.txt" are about 67.3797% similar.
"../tlo-file-similarity/samples/Removed-2nd-Half.txt" and "../tlo-file-similarity/samples/Swapped-3rd-And-4th-Paragraphs.txt" are about 66.3102% similar.
"../tlo-file-similarity/samples/Moved-Some-Lines.txt" and "../tlo-file-similarity/samples/Moved-Some-Words.txt" are about 73.0159% similar.
"../tlo-file-similarity/samples/Moved-Some-Lines.txt" and "../tlo-file-similarity/samples/Original.txt" are about 80.3213% similar.
"../tlo-file-similarity/samples/Moved-Some-Lines.txt" and "../tlo-file-similarity/samples/Removed-Some-Lines.txt" are about 61.5385% similar.
"../tlo-file-similarity/samples/Moved-Some-Lines.txt" and "../tlo-file-similarity/samples/Swapped-3rd-And-4th-Paragraphs.txt" are about 69.0763% similar.
"../tlo-file-similarity/samples/Moved-Some-Words.txt" and "../tlo-file-similarity/samples/Original.txt" are about 89.243% similar.
"../tlo-file-similarity/samples/Moved-Some-Words.txt" and "../tlo-file-similarity/samples/Removed-Some-Lines.txt" are about 65.4709% similar.
"../tlo-file-similarity/samples/Moved-Some-Words.txt" and "../tlo-file-similarity/samples/Swapped-3rd-And-4th-Paragraphs.txt" are about 75.6972% similar.
"../tlo-file-similarity/samples/Original.txt" and "../tlo-file-similarity/samples/Removed-Some-Lines.txt" are about 74.5455% similar.
"../tlo-file-similarity/samples/Original.txt" and "../tlo-file-similarity/samples/Removed-Some-Words.txt" are about 50.4202% similar.
"../tlo-file-similarity/samples/Original.txt" and "../tlo-file-similarity/samples/Swapped-3rd-And-4th-Paragraphs.txt" are about 85.4839% similar.
"../tlo-file-similarity/samples/Removed-Some-Lines.txt" and "../tlo-file-similarity/samples/Swapped-3rd-And-4th-Paragraphs.txt" are about 63.6364% similar.
```

## CMake Options

* TLO\_FS\_COLORED\_DIAGNOSTICS
    * Tell the compiler to use colors in diagnostics (GNU/Clang only)
    * On by default
* TLO\_FS\_USE\_LIBCPP
    * Use libc++ (Clang only)
    * Off by default
* TLO\_FS\_LINK\_FS
    * Link to filesystem library of older GNU and Clang (GNU/Clang only)
    * Prior to LLVM 9, using `std::filesystem` required linker option `-lc++fs`
    * Prior to GCC 9, using `std::filesystem` required linker option
      `-lstdc++fs`
    * Off by default
* TLO\_FS\_SQLITE3\_INCLUDE\_DIRS and TLO\_FS\_SQLITE3\_LIBRARIES
    * If both are specified (non-empty strings), will search for SQLite 3
      headers in the directories specified by TLO\_FS\_SQLITE3\_INCLUDE\_DIRS
      and will link to the libraries specified by TLO\_FS\_SQLITE3\_LIBRARIES
    * Otherwise, `find_package(SQLite3 REQUIRED)` will be used instead
    * Empty strings by default

## Program Options

### tlo-fuzzy-hash

```
$ ./tlo-fuzzy-hash
Usage: tlo-fuzzy-hash [options] <file or directory>...

Options:
  --database=value
    Store hashes in and get hashes from the database at the specified path (default: no database used).

  --num-threads=value
    Number of threads the program will use (default: 1).

  --print-status
    Allow program to print status updates to stderr (default: off).
```

### tlo-find-similar-hashes

```
$ ./tlo-find-similar-hashes
Usage: tlo-find-similar-hashes [options] <text file with hashes>...

Options:
  --num-threads=value
    Number of threads the program will use (default: 1).

  --output-format=value
    Set output format to regular, csv (comma-separated values), or tsv (tab-separated values) (default: regular).

  --print-status
    Allow program to print status updates to stderr (default: off).

  --similarity-threshold=value
    Display only the file pairs with a similarity score greater than or equal to this threshold (default: 50).
```

### Relevant Papers and Projects
* ["Identifying Almost Identical Files Using Context Triggered Piecewise
  Hashing"](https://www.dfrws.org/sites/default/files/session-files/paper-identifying_almost_identical_files_using_context_triggered_piecewise_hashing.pdf)
  by Jesse Kornblum (2006)
* [ssdeep - Fuzzy hashing program](https://ssdeep-project.github.io/ssdeep/index.html)
* [spamsum - A tool for generating and testing signatures on
  files](https://www.samba.org/ftp/unpacked/junkcode/spamsum/)

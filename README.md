# tlo-file-similarity

Command-line programs that use fuzzy hashing to compare files for similarity and find files that are similar to each other.

## Build Requirements

* CMake
* C++17 development environment for which CMake can generate build files
* SQLite 3

## Clone, Build, and Run

Clone into tlo-file-similarity directory.

```
$ git clone --branch develop --recursive git@github.com:OOZZY/tlo-file-similarity.git
```

Build.

```
$ mkdir build
$ cd build
$ cmake -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug ../tlo-file-similarity
$ make
```

Fuzzy hash all files in the current directory and all its subdirectories. In this example, the current directory is the CMake build directory.

```
$ ./tlo-fuzzy-hash . > hashes.txt
```

Compare hashes to find similar files.

```
$ ./tlo-find-similar-hashes hashes.txt
"./CMakeFiles/tlo-find-similar-hashes.dir/flags.make" and "./CMakeFiles/tlo-fuzzy-hash.dir/flags.make" are about 100% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/flags.make" and "./CMakeFiles/lcs-test.dir/flags.make" are about 100% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/flags.make" and "./CMakeFiles/lcs-test.dir/depend.internal" are about 50.3226% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/flags.make" and "./CMakeFiles/tlofs-core.dir/flags.make" are about 100% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/cmake_clean.cmake" and "./CMakeFiles/tlo-fuzzy-hash.dir/cmake_clean.cmake" are about 70.5357% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/cmake_clean.cmake" and "./CMakeFiles/lcs-test.dir/cmake_clean.cmake" are about 63.1111% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/cmake_clean.cmake" and "./CMakeFiles/tlofs-core.dir/cmake_clean.cmake" are about 52.1739% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/depend.internal" and "./CMakeFiles/tlo-find-similar-hashes.dir/link.txt" are about 55.2632% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/depend.internal" and "./CMakeFiles/tlo-fuzzy-hash.dir/depend.internal" are about 78.6517% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/depend.internal" and "./CMakeFiles/lcs-test.dir/depend.internal" are about 60.8187% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/depend.internal" and "./CMakeFiles/lcs-test.dir/depend.make" are about 54.0816% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/depend.internal" and "./CMakeFiles/tlo-find-similar-hashes.dir/depend.make" are about 62.9921% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/link.txt" and "./CMakeFiles/tlo-fuzzy-hash.dir/link.txt" are about 50% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/progress.make" and "./CMakeFiles/tlo-fuzzy-hash.dir/progress.make" are about 74.0741% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/progress.make" and "./CMakeFiles/lcs-test.dir/progress.make" are about 76.1905% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/flags.make" and "./CMakeFiles/lcs-test.dir/flags.make" are about 100% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/flags.make" and "./CMakeFiles/lcs-test.dir/depend.internal" are about 50.3226% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/flags.make" and "./CMakeFiles/tlofs-core.dir/flags.make" are about 100% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/cmake_clean.cmake" and "./CMakeFiles/lcs-test.dir/cmake_clean.cmake" are about 80.4598% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/cmake_clean.cmake" and "./CMakeFiles/tlofs-core.dir/cmake_clean.cmake" are about 62.963% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/depend.internal" and "./CMakeFiles/lcs-test.dir/depend.internal" are about 64.5161% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/depend.internal" and "./CMakeFiles/lcs-test.dir/depend.make" are about 55.5556% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/depend.internal" and "./CMakeFiles/tlo-fuzzy-hash.dir/depend.make" are about 72.7273% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/link.txt" and "./CMakeFiles/lcs-test.dir/link.txt" are about 60.8696% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/progress.make" and "./CMakeFiles/lcs-test.dir/progress.make" are about 76.1905% similar.
"./CMakeFiles/lcs-test.dir/flags.make" and "./CMakeFiles/lcs-test.dir/depend.internal" are about 50.3226% similar.
"./CMakeFiles/lcs-test.dir/flags.make" and "./CMakeFiles/tlofs-core.dir/flags.make" are about 100% similar.
"./CMakeFiles/lcs-test.dir/cmake_clean.cmake" and "./CMakeFiles/tlofs-core.dir/cmake_clean.cmake" are about 59.4595% similar.
"./CMakeFiles/lcs-test.dir/CXX.includecache" and "./CMakeFiles/tlo-find-similar-hashes.dir/CXX.includecache" are about 53.8012% similar.
"./CMakeFiles/lcs-test.dir/CXX.includecache" and "./CMakeFiles/tlo-fuzzy-hash.dir/CXX.includecache" are about 56.4417% similar.
"./CMakeFiles/lcs-test.dir/depend.internal" and "./CMakeFiles/lcs-test.dir/depend.make" are about 82.0809% similar.
"./CMakeFiles/lcs-test.dir/depend.internal" and "./CMakeFiles/tlofs-core.dir/flags.make" are about 50.3226% similar.
"./CMakeFiles/lcs-test.dir/depend.internal" and "./CMakeFiles/tlo-fuzzy-hash.dir/depend.make" are about 57.5758% similar.
"./CMakeFiles/lcs-test.dir/depend.make" and "./CMakeFiles/tlo-fuzzy-hash.dir/depend.make" are about 63.0137% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/CXX.includecache" and "./CMakeFiles/tlo-fuzzy-hash.dir/CXX.includecache" are about 82.5243% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/DependInfo.cmake" and "./CMakeFiles/tlo-fuzzy-hash.dir/DependInfo.cmake" are about 87.931% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/DependInfo.cmake" and "./CMakeFiles/lcs-test.dir/DependInfo.cmake" are about 86.0759% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/DependInfo.cmake" and "./CMakeFiles/tlofs-core.dir/DependInfo.cmake" are about 68.7898% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/DependInfo.cmake" and "./CMakeFiles/lcs-test.dir/DependInfo.cmake" are about 94.0639% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/DependInfo.cmake" and "./CMakeFiles/tlofs-core.dir/DependInfo.cmake" are about 76.0563% similar.
"./CMakeFiles/lcs-test.dir/DependInfo.cmake" and "./CMakeFiles/tlofs-core.dir/DependInfo.cmake" are about 74.8299% similar.
"./CMakeFiles/tlofs-core.dir/depend.internal" and "./CMakeFiles/tlofs-core.dir/depend.make" are about 50% similar.
"./CMakeFiles/tlo-find-similar-hashes.dir/build.make" and "./CMakeFiles/tlo-fuzzy-hash.dir/build.make" are about 52.2822% similar.
"./CMakeFiles/tlo-fuzzy-hash.dir/build.make" and "./CMakeFiles/lcs-test.dir/build.make" are about 60.5714% similar.
"./CMakeFiles/3.13.4/CMakeDetermineCompilerABI_C.bin" and "./CMakeFiles/3.13.4/CMakeDetermineCompilerABI_CXX.bin" are about 76.7677% similar.
"./CMakeFiles/3.13.4/CMakeDetermineCompilerABI_C.bin" and "./CMakeFiles/3.13.4/CompilerIdC/a.out" are about 50% similar.
"./CMakeFiles/3.13.4/CompilerIdCXX/a.out" and "./CMakeFiles/3.13.4/CMakeDetermineCompilerABI_CXX.bin" are about 54% similar.
"./CMakeFiles/3.13.4/CompilerIdCXX/a.out" and "./CMakeFiles/3.13.4/CompilerIdC/a.out" are about 63.3663% similar.
"./CMakeFiles/3.13.4/CMakeDetermineCompilerABI_CXX.bin" and "./CMakeFiles/3.13.4/CompilerIdC/a.out" are about 55.6701% similar.
"./CMakeFiles/3.13.4/CompilerIdCXX/CMakeCXXCompilerId.cpp" and "./CMakeFiles/3.13.4/CompilerIdC/CMakeCCompilerId.c" are about 86.2534% similar.
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
    * Prior to GCC 9, using `std::filesystem` required linker option `-lstdc++fs`
    * Off by default

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

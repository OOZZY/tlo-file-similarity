cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project("tlo-file-similarity")
enable_testing()

# Threads required. See CMake documentation on FindThreads module.
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(global_compile_options -pedantic -Wall -Wextra -Werror)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(global_compile_options ${global_compile_options} -Weverything
    -Wno-c++98-compat -Wno-padded -Wno-exit-time-destructors
    -Wno-global-constructors -Wno-weak-vtables)
endif()

set(global_link_libraries Threads::Threads)

option(TLOFS_COLORED_DIAGNOSTICS
  "Tell the compiler to use colors in diagnostics (GNU/Clang only)." ON)
if (TLOFS_COLORED_DIAGNOSTICS)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(global_compile_options ${global_compile_options} -fdiagnostics-color)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(global_compile_options ${global_compile_options} -fcolor-diagnostics)
  endif()
endif()

option(TLOFS_USE_LIBCPP "Use libc++ (Clang only)." OFF)
if (TLOFS_USE_LIBCPP)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(global_compile_options ${global_compile_options} -stdlib=libc++)
    set(global_link_libraries ${global_link_libraries} c++)
  endif()
endif()

option(TLOFS_LINK_FS
  "Link to filesystem library of older GNU and Clang (GNU/Clang only)." OFF)
if (TLOFS_LINK_FS)
  if (TLOFS_USE_LIBCPP)
    # Prior to LLVM 9, using std::filesystem required linker option -lc++fs.
    set(global_link_libraries ${global_link_libraries} c++fs)
  else()
    # Prior to GCC 9, using std::filesystem required linker option -lstdc++fs.
    set(global_link_libraries ${global_link_libraries} stdc++fs)
  endif()
endif()

set(core_headers filesystem.hpp fuzzy.hpp lcs.hpp options.hpp print.hpp
  string.hpp)
set(core_sources filesystem.cpp fuzzy.cpp lcs.cpp options.cpp string.cpp)

add_library(tlofs-core STATIC ${core_headers} ${core_sources})
set_target_properties(tlofs-core PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(tlofs-core PUBLIC cxx_std_17)
target_compile_options(tlofs-core PRIVATE ${global_compile_options})
target_link_libraries(tlofs-core PUBLIC ${global_link_libraries})

add_executable(lcs-test lcs-test.cpp)
set_target_properties(lcs-test PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(lcs-test PRIVATE ${global_compile_options})
target_link_libraries(lcs-test PRIVATE tlofs-core)

add_test(NAME lcs-test COMMAND lcs-test)

add_executable(tlo-fuzzy-hash tlo-fuzzy-hash.cpp)
set_target_properties(tlo-fuzzy-hash PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tlo-fuzzy-hash PRIVATE ${global_compile_options})
target_link_libraries(tlo-fuzzy-hash PRIVATE tlofs-core)

add_executable(tlo-find-similar-hashes tlo-find-similar-hashes.cpp)
set_target_properties(tlo-find-similar-hashes PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tlo-find-similar-hashes PRIVATE ${global_compile_options})
target_link_libraries(tlo-find-similar-hashes PRIVATE tlofs-core)

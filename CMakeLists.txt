cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project("tlo-file-similarity")
enable_testing()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(global_compile_options -pedantic -Wall -Wextra -Werror)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(global_compile_options ${global_compile_options} -Weverything
    -Wno-c++98-compat)
endif()

option(TLOFS_COLORED_DIAGNOSTICS
  "Tell the compiler to use colors in diagnostics (GNU/Clang only)." ON)
if (TLOFS_COLORED_DIAGNOSTICS)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(global_compile_options ${global_compile_options} -fdiagnostics-color)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(global_compile_options ${global_compile_options} -fcolor-diagnostics)
  endif()
endif()

set(core_headers filesystem.hpp fuzzy.hpp lcs.hpp similarity.hpp string.hpp)
set(core_sources filesystem.cpp fuzzy.cpp lcs.cpp similarity.cpp string.cpp)

add_library(tlofs-core STATIC ${core_headers} ${core_sources})
set_target_properties(tlofs-core PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(tlofs-core PUBLIC cxx_std_17)
target_compile_options(tlofs-core PRIVATE ${global_compile_options})

add_executable(lcs-test lcs-test.cpp)
set_target_properties(lcs-test PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(lcs-test PRIVATE ${global_compile_options})
target_link_libraries(lcs-test PRIVATE tlofs-core)

add_test(NAME lcs-test COMMAND lcs-test)

add_executable(tlo-check-similarity tlo-check-similarity.cpp)
set_target_properties(tlo-check-similarity PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tlo-check-similarity PRIVATE ${global_compile_options})
target_link_libraries(tlo-check-similarity PRIVATE tlofs-core)

add_executable(tlo-find-similar-files tlo-find-similar-files.cpp)
set_target_properties(tlo-find-similar-files PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tlo-find-similar-files PRIVATE ${global_compile_options})
target_link_libraries(tlo-find-similar-files PRIVATE tlofs-core)

add_executable(tlo-fuzzy-hash tlo-fuzzy-hash.cpp)
set_target_properties(tlo-fuzzy-hash PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tlo-fuzzy-hash PRIVATE ${global_compile_options})
target_link_libraries(tlo-fuzzy-hash PRIVATE tlofs-core)

add_executable(tlo-find-similar-hashes tlo-find-similar-hashes.cpp)
set_target_properties(tlo-find-similar-hashes PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tlo-find-similar-hashes PRIVATE ${global_compile_options})
target_link_libraries(tlo-find-similar-hashes PRIVATE tlofs-core)

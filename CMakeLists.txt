cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project("tlo-file-similarity")

option(TLO_FS_COLORED_DIAGNOSTICS
  "Tell the compiler to use colors in diagnostics (GNU/Clang only)." ON)
set(TLO_CPP_COLORED_DIAGNOSTICS ${TLO_FS_COLORED_DIAGNOSTICS})

option(TLO_FS_USE_LIBCPP "Use libc++ (Clang only)." OFF)
set(TLO_CPP_USE_LIBCPP ${TLO_FS_USE_LIBCPP})

option(TLO_FS_LINK_FS
  "Link to filesystem library of older GNU and Clang (GNU/Clang only)." OFF)
set(TLO_CPP_LINK_FS ${TLO_FS_LINK_FS})

add_subdirectory(tlo-cpp)

set(private_compile_options "")
set(public_link_libraries tlo-cpp)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  list(APPEND private_compile_options -pedantic -Wall -Wextra -Werror)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  list(APPEND private_compile_options -Weverything -Wno-c++98-compat
    -Wno-weak-vtables -Wno-padded -Wno-exit-time-destructors
    -Wno-global-constructors)
endif()

if (TLO_FS_COLORED_DIAGNOSTICS)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    list(APPEND private_compile_options -fdiagnostics-color)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    list(APPEND private_compile_options -fcolor-diagnostics)
  endif()
endif()

set(tlo_file_similarity_headers compare.hpp database.hpp fuzzy.hpp)
set(tlo_file_similarity_sources compare.cpp database.cpp fuzzy.cpp)

add_library(tlo-file-similarity STATIC ${tlo_file_similarity_headers}
  ${tlo_file_similarity_sources})
set_target_properties(tlo-file-similarity PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(tlo-file-similarity PUBLIC cxx_std_17)
target_compile_options(tlo-file-similarity PRIVATE ${private_compile_options})
target_link_libraries(tlo-file-similarity PUBLIC ${public_link_libraries})

add_executable(tlo-fuzzy-hash tlo-fuzzy-hash.cpp)
set_target_properties(tlo-fuzzy-hash PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tlo-fuzzy-hash PRIVATE ${private_compile_options})
target_link_libraries(tlo-fuzzy-hash PRIVATE tlo-file-similarity)

add_executable(tlo-find-similar-hashes tlo-find-similar-hashes.cpp)
set_target_properties(tlo-find-similar-hashes PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tlo-find-similar-hashes PRIVATE
  ${private_compile_options})
target_link_libraries(tlo-find-similar-hashes PRIVATE tlo-file-similarity)

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project("tlo-file-similarity")

option(TLO_FS_COLORED_DIAGNOSTICS
  "Tell the compiler to use colors in diagnostics (GNU/Clang only)." ON
)
set(TLO_CPP_COLORED_DIAGNOSTICS ${TLO_FS_COLORED_DIAGNOSTICS})

option(TLO_FS_USE_LIBCPP "Use libc++ (Clang only)." OFF)
set(TLO_CPP_USE_LIBCPP ${TLO_FS_USE_LIBCPP})

option(TLO_FS_LINK_FS
  "Link to filesystem library of older GNU and Clang (GNU/Clang only)." OFF
)
set(TLO_CPP_LINK_FS ${TLO_FS_LINK_FS})

macro(string_option name docstring default_value)
  if (NOT DEFINED ${name} OR DEFINED CACHE{${name}})
    set(${name} "${default_value}" CACHE STRING "${docstring}")
  endif()
endmacro(string_option)

string_option(TLO_FS_SQLITE3_INCLUDE_DIRS
  "Include directories containing SQLite 3 headers." ""
)
set(TLO_CPP_SQLITE3_INCLUDE_DIRS ${TLO_FS_SQLITE3_INCLUDE_DIRS})

string_option(TLO_FS_SQLITE3_LIBRARIES
  "Libraries to link against to use SQLite 3." ""
)
set(TLO_CPP_SQLITE3_LIBRARIES ${TLO_FS_SQLITE3_LIBRARIES})

add_subdirectory(tlo-cpp)

set(private_compile_options "")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  list(APPEND private_compile_options
    -pedantic
    -Wall
    -Werror
    -Wextra
  )
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  list(APPEND private_compile_options
    -Weverything
    -Wno-c++98-compat
    -Wno-exit-time-destructors
    -Wno-global-constructors
    -Wno-padded
  )
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  list(APPEND private_compile_options
    /experimental:external
    /external:anglebrackets
    /external:templates-
    /external:W0
    /Wall
    /wd4514
    /wd4571
    /wd4623
    /wd4625
    /wd4626
    /wd4710
    /wd4711
    /wd4820
    /wd5026
    /wd5027
    /wd5045
    /WX
  )
endif()

if (TLO_FS_COLORED_DIAGNOSTICS)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    list(APPEND private_compile_options -fdiagnostics-color)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    list(APPEND private_compile_options -fcolor-diagnostics)
  endif()
endif()

macro(prepend listVar prefix)
  set(${listVar} "")
  foreach(item ${ARGN})
    list(APPEND ${listVar} "${prefix}${item}")
  endforeach(item)
endmacro(prepend)

set(tlo_file_similarity_headers
  compare.hpp
  database.hpp
  fuzzy.hpp
)
prepend(tlo_file_similarity_headers
  include/tlo-file-similarity/ ${tlo_file_similarity_headers}
)

set(tlo_file_similarity_sources
  compare.cpp
  database.cpp
  fuzzy.cpp
)
prepend(tlo_file_similarity_sources src/ ${tlo_file_similarity_sources})

add_library(tlo-file-similarity STATIC
  ${tlo_file_similarity_headers}
  ${tlo_file_similarity_sources}
)
set_target_properties(tlo-file-similarity PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(tlo-file-similarity PUBLIC cxx_std_17)
target_compile_options(tlo-file-similarity PRIVATE ${private_compile_options})
target_include_directories(tlo-file-similarity PUBLIC include)
target_link_libraries(tlo-file-similarity PUBLIC tlo-cpp)

add_executable(tlo-fuzzy-hash src/tlo-fuzzy-hash.cpp)
set_target_properties(tlo-fuzzy-hash PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tlo-fuzzy-hash PRIVATE ${private_compile_options})
target_link_libraries(tlo-fuzzy-hash PRIVATE tlo-file-similarity)

add_executable(tlo-find-similar-hashes src/tlo-find-similar-hashes.cpp)
set_target_properties(tlo-find-similar-hashes PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tlo-find-similar-hashes
  PRIVATE ${private_compile_options}
)
target_link_libraries(tlo-find-similar-hashes PRIVATE tlo-file-similarity)

install(DIRECTORY include/tlo-file-similarity DESTINATION include)
install(TARGETS tlo-file-similarity DESTINATION lib)
install(TARGETS tlo-fuzzy-hash tlo-find-similar-hashes DESTINATION bin)

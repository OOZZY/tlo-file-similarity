cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project("tlo-file-similarity")

set(docstring
  "Tell the compiler to use colors in diagnostics (GNU/Clang only)."
)
option(TLO_FS_COLORED_DIAGNOSTICS ${docstring} ON)
set(TLO_CPP_COLORED_DIAGNOSTICS ${TLO_FS_COLORED_DIAGNOSTICS}
  CACHE BOOL ${docstring}
)

set(docstring "Use libc++ (Clang only).")
option(TLO_FS_USE_LIBCPP ${docstring} OFF)
set(TLO_CPP_USE_LIBCPP ${TLO_FS_USE_LIBCPP} CACHE BOOL ${docstring})

set(docstring
  "Link to filesystem library of older GNU and Clang (GNU/Clang only)."
)
option(TLO_FS_LINK_FS ${docstring} OFF)
set(TLO_CPP_LINK_FS ${TLO_FS_LINK_FS} CACHE BOOL ${docstring})

set(docstring "Include directories containing SQLite 3 headers.")
set(TLO_FS_SQLITE3_INCLUDE_DIRS "" CACHE STRING ${docstring})
set(TLO_CPP_SQLITE3_INCLUDE_DIRS ${TLO_FS_SQLITE3_INCLUDE_DIRS}
  CACHE STRING ${docstring}
)

set(docstring "Libraries to link against to use SQLite 3.")
set(TLO_FS_SQLITE3_LIBRARIES "" CACHE STRING ${docstring})
set(TLO_CPP_SQLITE3_LIBRARIES ${TLO_FS_SQLITE3_LIBRARIES}
  CACHE STRING ${docstring}
)

add_subdirectory(tlo-cpp)

set(private_compile_options "")
set(public_link_libraries tlo-cpp)

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
  ${PROJECT_SOURCE_DIR}/include/tlo-file-similarity/
  ${tlo_file_similarity_headers}
)

set(tlo_file_similarity_sources
  compare.cpp
  database.cpp
  fuzzy.cpp
)
prepend(tlo_file_similarity_sources
  ${PROJECT_SOURCE_DIR}/src/ ${tlo_file_similarity_sources}
)

add_library(tlo-file-similarity STATIC
  ${tlo_file_similarity_headers}
  ${tlo_file_similarity_sources}
)
set_target_properties(tlo-file-similarity PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(tlo-file-similarity PUBLIC cxx_std_17)
target_compile_options(tlo-file-similarity PRIVATE ${private_compile_options})
target_include_directories(tlo-file-similarity
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(tlo-file-similarity PUBLIC ${public_link_libraries})

add_executable(tlo-fuzzy-hash ${PROJECT_SOURCE_DIR}/src/tlo-fuzzy-hash.cpp)
set_target_properties(tlo-fuzzy-hash PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tlo-fuzzy-hash PRIVATE ${private_compile_options})
target_link_libraries(tlo-fuzzy-hash PRIVATE tlo-file-similarity)

add_executable(tlo-find-similar-hashes
  ${PROJECT_SOURCE_DIR}/src/tlo-find-similar-hashes.cpp
)
set_target_properties(tlo-find-similar-hashes PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(tlo-find-similar-hashes
  PRIVATE ${private_compile_options}
)
target_link_libraries(tlo-find-similar-hashes PRIVATE tlo-file-similarity)

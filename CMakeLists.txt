cmake_minimum_required (VERSION 3.10)

project (mapredo)
set(PROJECT_VERSION 0.7.5)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
if (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -g -pedantic -Wall")
endif()

include (CheckIncludeFiles)
check_include_files (unistd.h HAVE_UNISTD_H)
if(NOT HAVE_UNISTD_H)
  check_include_files (io.h HAVE_IO_H)
endif()
include (CheckFunctionExists)
check_function_exists (strerror_r HAVE_STRERROR_R)
if(HAVE_STRERROR_R)
  include (CheckCXXSourceCompiles)
  check_cxx_source_compiles("#include <cstring>
int main() {char b[80];char* s = strerror_r(0,b,80);return 0;}"
    HAVE_GNU_STRERROR_R)
else()
  check_function_exists (strerror_s HAVE_STRERROR_S)
endif()
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/mapredo/config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/mapredo/config.h)

include(CheckIncludeFiles)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/mapredo/config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/mapredo/config.h)

include_directories (BEFORE ${PROJECT_SOURCE_DIR})

set (ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${PROJECT_VERSION})
add_custom_target (dist
  COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
  | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

include (GNUInstallDirs)

configure_file (
  "${mapredo_SOURCE_DIR}/mapredo.spec.in"
  "${mapredo_BINARY_DIR}/mapredo.spec" @ONLY)

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})
add_custom_target(check COMMAND "tests/unittests")

find_package(Doxygen)
find_package(Ruby)

add_definitions(-DPLUGINSDIR="${CMAKE_INSTALL_FULL_LIBDIR}/mapredo")

add_subdirectory (frontend)
add_subdirectory (mapredo)
add_subdirectory (plugins)
add_subdirectory (tests)
add_subdirectory (doc)
add_subdirectory (ruby)

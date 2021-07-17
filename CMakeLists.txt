# Copyright 2021 by Daniel Winkelman. All rights reserved.

cmake_minimum_required (VERSION 3.16)

set (CMAKE_BUILD_TYPE Debug)
set (CMAKE_CXX_STANDARD 17)

# Define targets
project (Clef CXX)
set (Target_util_tests clef-util-tests)

# Install Google Test
include (FetchContent)
FetchContent_Declare (googletest URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip)
FetchContent_MakeAvailable (googletest)
enable_testing ()

# Set up include directories
include_directories (src)
include_directories (${googletest_SOURCE_DIR}/googletest/include)

# Util testing target
file (GLOB UTIL_TESTS_SOURCES tests/main.cc tests/util/*.cc)
add_executable (${Target_util_tests} ${UTIL_TESTS_SOURCES})
target_link_libraries (${Target_util_tests} gtest_main)

# Set up for Google Test
include (GoogleTest)
gtest_discover_tests (${Target_util_tests})

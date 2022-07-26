# SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only
cmake_minimum_required(VERSION 3.10)
project(rootsimcc LANGUAGES C CXX)
set(PROJECT_VERSION 3.0.0-alpha.3)
set(PROJECT_DESCRIPTION "The ROOT-Sim C Compiler")
set(CMAKE_VERBOSE_MAKEFILE ON)

include(ExternalProject)

add_definitions(-DROOTSIM_VERSION=${PROJECT_VERSION})
add_compile_options(-Wall -Wextra -pedantic)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
find_package(MPI REQUIRED)

add_subdirectory(src)

# Run the tests
enable_testing()
add_subdirectory(test)

# Generate and inspect documentation
add_subdirectory(docs)

ExternalProject_Add(core
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/subprojects/core
    CMAKE_ARGS -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    TEST_BEFORE_INSTALL 1
)
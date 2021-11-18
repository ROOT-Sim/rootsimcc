cmake_minimum_required(VERSION 3.10)
project(rootsimcc LANGUAGES C CXX)
set(PROJECT_VERSION 3.0.0-alpha.3)
set(PROJECT_DESCRIPTION "The ROOT-Sim C Compiler")
set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-DROOTSIM_VERSION=${PROJECT_VERSION})

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
find_package(CLANG REQUIRED CONFIG)

include_directories(src)

# Build the compiler, the plugins, the userspace libraries
add_executable(rootsimcc
        src/compiler/rootsimcc.c
        src/datatypes/dynstr.c
)

add_library(rsmeminstr SHARED
        src/plugins/rootsim-cc_llvm.cpp
)

add_library(rsmodel STATIC
        src/lib/config/jsmn.c
        src/lib/config/reflect.c
        src/datatypes/vector.c
        src/lib/argparse/argparse.c
)

# Run the tests
enable_testing()
add_subdirectory(test)

# Generate and inspect documentation
add_subdirectory(docs)

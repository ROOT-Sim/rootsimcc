# Build the compiler, the plugins, the userspace libraries

add_executable(compiler compiler/compiler.cpp)

string (REPLACE ";" " -I" MPI_INCLUDES "${MPI_C_INCLUDE_DIRS}")
string (REPLACE ";" " -l" MPI_LIBS "${MPI_C_LIBRARIES}")
string (REPLACE ";" " " MPI_COMPILE_DEFINITIONS "${MPI_C_COMPILE_DEFINITIONS}")

target_compile_definitions(compiler PRIVATE
        ROOTSIM_LIB_DIR="${CMAKE_INSTALL_PREFIX}/lib"
        ROOTSIM_INC_DIR="${CMAKE_INSTALL_PREFIX}/include"
        MPI_LIBS="${MPI_LIBS}"
        MPI_INCLUDES_FLAGS="-I${MPI_INCLUDES}")

add_library(rsllvmwrap SHARED plugin/wrap.cpp)
target_include_directories(rsllvmwrap PRIVATE . ${LLVM_INCLUDE_DIRS})
# Disable the warning for unused function parameters: LLVM codebase is riddled with those
target_compile_options(rsllvmwrap PRIVATE -Wno-unused-parameter)

if(${LLVM_PACKAGE_VERSION} VERSION_GREATER_EQUAL "14")
    target_compile_definitions(compiler PRIVATE LLVM_USE_NEW_PASS_MANAGER)
    target_compile_definitions(rsllvmwrap PRIVATE LLVM_USE_NEW_PASS_MANAGER)
endif()

add_library(rsmodel STATIC model_lib/argparse.c model_lib/init.c model_lib/model_main.c)
target_include_directories(rsmodel PRIVATE . ${CMAKE_SOURCE_DIR}/subprojects/core/src)

install(TARGETS compiler rsllvmwrap rsmodel
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
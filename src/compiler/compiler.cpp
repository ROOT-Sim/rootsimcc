/**
 * @file compiler/compiler.cpp
 *
 * @brief The ROOT-Sim compiler
 *
 * This is the ROOT-Sim compiler, a compiler wrapper which allows to setup all necessary includes and configurations to
 * run with a parallel or distributed simulation. This is targeting low level C models.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#ifndef ROOTSIM_OPTIMIZATION_OPTIONS
/// The optimization options to be used when compiling models
/** This macro is filled in at build time */
#define ROOTSIM_OPTIMIZATION_OPTIONS "-g"
#endif

#ifndef ROOTSIM_CC
/// The path of the C compiler to use when compiling models
/** This macro is filled in at build time */
#define ROOTSIM_CC "clang"
#endif

#ifndef MPI_INCLUDES_FLAGS
/// The path of the C compiler to use when compiling models
/** This macro is filled in at build time */
#define MPI_INCLUDES_FLAGS ""
#endif

#ifndef MPI_LIBS
/// The path of the C compiler to use when compiling models
/** This macro is filled in at build time */
#define MPI_LIBS ""
#endif

#ifndef ROOTSIM_LIB_DIR
/// The path of the installed ROOT-Sim libraries
/** This macro is filled in at build time */
#define ROOTSIM_LIB_DIR ""
#endif

#ifndef ROOTSIM_INC_DIR
/// The path of the installed ROOT-Sim headers
/** This macro is filled in at build time */
#define ROOTSIM_INC_DIR ""
#endif

static const char *options[] = {"-rsinc", "-rslib"};

static int extract_options(const char *const *args, const char **parsed_options, std::string &remaining)
{
	if(*args == nullptr) {
		std::cerr << "No input files\n";
		return -1;
	}

	do {
		for(unsigned i = 0; i < sizeof(options) / sizeof(*options); ++i) {
			if(std::strcmp(*args, options[i]) == 0) {
				parsed_options[i] = *(++args);
				if(parsed_options[i] == nullptr) {
					std::cerr << "Option " + std::string(options[i]) + " is missing its argument\n";
					return -1;
				}
			}
		}
		remaining.append(" ");
		remaining.append(*args);
		++args;
	} while(*args);
	return 0;
}

/**
 * @brief The main entry point of the custom compiler
 * @param argc The count of command line arguments, as per ISO C standard
 * @param argv The list of command line arguments, as per ISO C standard
 */
int main(int argc, char *argv[])
{
	(void)argc;
	const char *parsed_options[sizeof(options) / sizeof(*options)] = {ROOTSIM_INC_DIR, ROOTSIM_LIB_DIR};

	std::string command_line(ROOTSIM_CC);
	if(extract_options(&argv[1], parsed_options, command_line) < 0)
		return EXIT_FAILURE;

	command_line.append(" " MPI_INCLUDES_FLAGS " " ROOTSIM_OPTIMIZATION_OPTIONS);

	command_line.append(" -I");
	command_line.append(parsed_options[0]);

	command_line.append(" -L");
	command_line.append(parsed_options[1]);
#ifdef LLVM_USE_NEW_PASS_MANAGER
	command_line.append(" -fpass-plugin=");
#else
	command_line.append(" -flegacy-pass-manager -Xclang -load -Xclang ");
#endif
	command_line.append(parsed_options[1]);
	command_line.append("/librsllvmwrap.so -Wl,--as-needed -lrscore -lrsmodel -lm -lpthread " MPI_LIBS);

	return std::system(command_line.c_str());
}

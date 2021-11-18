/**
 * @file compiler.c
 *
 * @brief The ROOT-Sim compiler
 *
 * This is the ROOT-Sim compiler, a compiler wrapper which allows
 * to setup all necessary includes and configurations to run with
 * a parallel or distributed simulation. This is targeting low
 * level C models.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include <datatypes/dynstr.h>

#ifndef ROOTSIM_OPTIMIZATION_OPTIONS
/// The optimization options to be used when compiling models
/** This macro is filled in at build time */
#define ROOTSIM_OPTIMIZATION_OPTIONS ""
#endif

#ifndef ROOTSIM_CC
/// The path of the C compiler to use when compiling models
/** This macro is filled in at build time */
#define ROOTSIM_CC ""
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

static const char *cmd_line_plugin = "-fplugin=%s/librootsim-cc_llvm.so ";
static const char *cmd_line_libraries = "-Wl,--as-needed -lrscore -lm -lpthread";

/// The path to the ROOT-Sim include directory. Can be overridden with -rsinc at command line
const char *include_path = ROOTSIM_INC_DIR;
/// The path to the ROOT-Sim library path. Can be overridden with -rslib at command line
const char *lib_path = ROOTSIM_LIB_DIR;


/// The dynamic string to build the compiling command line
struct dynstr *command_line;

/// The dynamic string to keep user-specified arguments
struct dynstr *user_args;


/**
 * @brief The main entry point of the custom compiler
 * @param argc The count of command line arguments, as per ISO C standard
 * @param argv The list of command line arguments, as per ISO C standard
 */
int main(int argc, char **argv)
{
	(void)argc;
	int ret = 0;
	++argv;

	dynstr_init(&command_line, 512);
	dynstr_init(&user_args, 64);

	while (*argv) {
		if(strcmp(*argv, "-rsinc") == 0) {
			include_path = *(++argv);
			++argv;
			continue;
		}
		if(strcmp(*argv, "-rslib") == 0) {
			lib_path = *(++argv);
			++argv;
			continue;
		}

		dynstr_strcat(user_args, *argv, strlen(*argv));
		dynstr_strcat(user_args, " ", 1);
		++argv;
	}

	// Assemble the final command
	dynstr_printcat(command_line, "%s %s -I%s -L%s ", ROOTSIM_CC, ROOTSIM_OPTIMIZATION_OPTIONS, include_path, lib_path);
	dynstr_printcat(command_line, cmd_line_plugin, lib_path);
	dynstr_printcat(command_line, dynstr_getbuff(user_args), dynstr_getbuff(user_args));
	dynstr_printcat(command_line, cmd_line_libraries, strlen(cmd_line_libraries));

	if (system(dynstr_getbuff(command_line))) {
#ifndef NDEBUG
		fprintf(stderr, "Failed to run: %s\n", dynstr_getbuff(command_line));
#else
		fprintf(stderr, "Failed to run: " ROOTSIM_CC "\n");
#endif
		ret = -1;
	}

	dynstr_fini(&user_args);
	dynstr_fini(&command_line);
	return ret;
}

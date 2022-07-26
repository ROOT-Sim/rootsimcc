/**
 * @file model_lib/init.c
 *
 * @brief Initialization routines
 *
 * This module implements the simulator initialization routines
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <model_lib/init.h>

#include <model_lib/argparse.h>

#include <limits.h>
#include <memory.h>
#include <stdlib.h>

/// This is the list of arg_parse.h mnemonics for command line arguments
enum option_key { OPT_NPRC, OPT_LOG, OPT_SIMT, OPT_GVT, OPT_NP, OPT_BIND, OPT_SERIAL, OPT_LAST };

static struct simulation_configuration *current_config;

/// The array of ROOT-Sim supported command line options
static struct ap_option ap_options[] = {
    {"lp", OPT_NPRC, "VALUE", "Total number of Logical Processes being launched at simulation startup"},
    {"log-level", OPT_LOG, "TYPE", "Logging level"},
    {"time", OPT_SIMT, "VALUE", "Logical time at which the simulation will be considered completed"},
    {"gvt-period", OPT_GVT, "VALUE", "Time between two GVT reductions (in milliseconds)"},
    {"serial", OPT_SERIAL, NULL, "Runs a simulation with the serial runtime"},
    {"wt", OPT_NP, "VALUE", "Number of total cores being used by the simulation"},
    {"no-bind", OPT_BIND, NULL, "Disables thread to core binding"}, {0}};

/**
 * @brief Throws a formatted parsing error
 *
 * This macro uses symbols defined in parse_opt() body, therefore it can't be used anywhere else
 */
#define malformed_option_failure()                                                                                     \
	__extension__({                                                                                                \
		size_t __i;                                                                                            \
		for(__i = 0; ap_options[__i].key != key; ++__i)                                                        \
			;                                                                                              \
		arg_parse_error("invalid value \"%s\" in the %s option.", arg, ap_options[__i].name);                  \
	})

/**
 * @brief Parses the current argument into a unsigned value with bounds checks
 * @param low The minimum allowed value of the parsed value
 * @param high The maximum allowed value of the parsed value
 * @return the parsed unsigned long long value
 *
 * If an error occurs malformed_option_failure() is called accordingly.
 * This macro uses symbols defined in parse_opt() body, therefore it can't be used anywhere else
 */
#define parse_ullong_limits(low, high)                                                                                 \
	__extension__({                                                                                                \
		unsigned long long int __value;                                                                        \
		char *__endptr;                                                                                        \
		__value = strtoull(arg, &__endptr, 10);                                                                \
		if(*arg == '\0' || *__endptr != '\0' || __value < low || __value > high) {                             \
			malformed_option_failure();                                                                    \
		}                                                                                                      \
		__value;                                                                                               \
	})

/**
 * @brief Parses the current argument into a double value with bounds checks
 * @param low The minimum allowed value of the parsed value
 * @param high The maximum allowed value of the parsed value
 * @return the parsed double value
 *
 * If an error occurs malformed_option_failure() is called accordingly.
 * This macro uses symbols defined in parse_opt() body, therefore it can't be used anywhere else
 */
#define parse_ldouble_limits(low, high)                                                                                \
	__extension__({                                                                                                \
		long double __value;                                                                                   \
		char *__endptr;                                                                                        \
		__value = strtold(arg, &__endptr);                                                                     \
		if(*arg == '\0' || *__endptr != '\0' || __value < (low) || __value > (high)) {                         \
			malformed_option_failure();                                                                    \
		}                                                                                                      \
		__value;                                                                                               \
	})

/**
 * @brief Parses a single ROOT-Sim option, also handles parsing events
 * @param key the key identifying the currently parsed option or event
 * @param arg the command line argument supplied with the option if present
 *
 * This is used in ROOT-Sim struct ap_section
 */
static void parse_opt(int key, const char *arg)
{
	switch(key) {
	case OPT_NPRC:
		current_config->lps = parse_ullong_limits(1, UINT_MAX);
		break;

	case OPT_LOG:
		// TODO
		break;

	case OPT_SIMT:
		current_config->termination_time = parse_ldouble_limits(0, SIMTIME_MAX);
		break;

	case OPT_GVT:
		current_config->gvt_period = parse_ullong_limits(1, 10000) * 1000;
		break;

	case OPT_NP:
		current_config->n_threads = parse_ullong_limits(1, UINT_MAX);
		break;

	case OPT_BIND:
		current_config->core_binding = false;
		break;

	case OPT_SERIAL:
		current_config->serial = true;
		break;

	case AP_KEY_INIT:
		current_config->lps = 0;
		current_config->n_threads = 0;
		current_config->termination_time = SIMTIME_MAX;
		current_config->gvt_period = 200000;
		current_config->log_level = LOG_INFO;
		current_config->serial = false;
		current_config->core_binding = true;
		current_config->ckpt_interval = 0;
		break;

	case AP_KEY_FINI:
		if(current_config->lps == 0)
			arg_parse_error("number of LPs was not provided \"--lp\"");
	}
}

#undef parse_ullong_limits
#undef handle_string_option
#undef malformed_option_failure

__attribute__((weak)) struct ap_option model_options[] = {0};

__attribute__((weak)) void model_parse(int key, const char *arg)
{
	(void)key;
	(void)arg;
}

/// The struct ap_section containing ROOT-Sim internal parser and the model one
struct ap_section ap_sects[] = {{NULL, ap_options, parse_opt}, {"Model specific options", model_options, model_parse},
    {0}};

/// The struct ap_settings with the ROOT-Sim command line parsing configuration
struct ap_settings ap_sets = {"ROOT-Sim", "Proper version string", "piccione@diag.uniroma1.it", ap_sects};

/**
 * @brief Parses ROOT-Sim command line arguments
 * @param argc the argc value from the main() function
 * @param argv the argv value from the main() function
 * @param config a pointer to the configuration struct to fill in
 */
void init_args_parse(int argc, char *argv[], struct simulation_configuration *config)
{
	(void)argc;
	current_config = config;
	arg_parse_run(&ap_sets, argv);
}

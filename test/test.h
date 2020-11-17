#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <threads.h>

#define TEST_BAD_FAIL_EXIT_CODE 99

typedef struct {
    unsigned count;
    unsigned waiters;
    uint64_t sequence;
    mtx_t mutex;
    cnd_t condvar;
} barrier_t;

extern const struct _test_config_t {
	int (*test_init_fnc)(void);
	int (*test_fini_fnc)(void);
	int (*test_fnc)(void);
	unsigned threads_count;
	const char *expected_output;
	size_t expected_output_size;
	const char *test_name;
	const char **test_arguments;
} test_config;

extern FILE *test_output_file;

extern void test_abort(void);
extern bool test_thread_barrier(void);

// core.c mock
typedef uint64_t lp_id_t;
typedef unsigned rid_t;
typedef int nid_t;

extern lp_id_t n_lps;
extern nid_t n_nodes;
extern rid_t n_threads;
extern nid_t nid;
extern __thread rid_t rid;

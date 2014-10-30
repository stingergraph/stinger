/*
 * stinger_test.h
 *
 *  Created on: Oct 28, 2014
 *      Author: jason
 */

#ifndef STINGER_TEST_H_
#define STINGER_TEST_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include <stdint.h>
#include "stinger_core/stinger.h"
#include <stdio.h>

struct test_st;

typedef struct test_harness_st {
	int64_t num_tests;
	struct test_st * tests;
	int64_t current_test;
	int64_t next_test;
} test_harness_t;

typedef void testfunc_t(test_harness_t* th, void *);

typedef struct test_st {
	char * desc;
	size_t passed_test;
	size_t test_ran;
	testfunc_t * func;
	void * func_arg;
} test_t;

#define TEST_ASSERT_EQ(arg1, arg2) \
	do { \
		if (arg1 == arg2) { \
			th->tests[th->current_test].passed_test &= 1; \
		} else { \
			th->tests[th->current_test].passed_test &= 0; \
		} \
	} while (0);

#define TEST_ASSERT_NEQ(arg1, arg2) \
  do { \
    if (arg1 != arg2) { \
      th->tests[th->current_test].passed_test &= 1; \
    } else { \
      th->tests[th->current_test].passed_test &= 0; \
    } \
  } while (0);

#define TEST_ASSERT_STRCMP(arg1, arg2) \
	do { \
		if (!strcmp(arg1,arg2)) { \
			th->tests[th->current_test].passed_test &= 1; \
		} else { \
			th->tests[th->current_test].passed_test &= 0; \
		} \
	} while (0);

#define FAILED_TEST (th->tests[th->current_test].passed_test == 0)


test_harness_t * new_test_harness(int64_t num_tests);

size_t register_test(test_harness_t* th, char * desc, testfunc_t * func, void * func_arg);
void run_tests(test_harness_t* th, FILE * output);
void run_test(test_harness_t* th, int64_t test_num, FILE * output);
void print_test_summary(test_harness_t* th, FILE * output);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* STINGER_TEST_H_ */

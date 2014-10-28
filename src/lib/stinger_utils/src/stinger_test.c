/*
 * stinger_test.c
 *
 *  Created on: Oct 28, 2014
 *      Author: jason
 */

#define LOG_AT_D

#include "stinger_test.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include <stdio.h>

test_harness_t * new_test_harness(int64_t num_tests) {
	test_harness_t * th = (test_harness_t *)xcalloc(1, sizeof(test_harness_t));
	th->tests = (test_t *)xcalloc(num_tests, sizeof(test_t));
	th->num_tests = num_tests;
	for (int64_t i=0; i < num_tests; i++) {
		th->tests[i].desc = NULL;
		th->tests[i].test_type = TEST_TYPE_INT;
		th->tests[i].expected_val.int_val = 0;
		th->tests[i].test_val.int_val = 0;
		th->tests[i].passed_test = -1;
	}
	return th;
}

void add_test_description(test_harness_t* th, int64_t test_num, char * desc) {
	th->tests[test_num].desc = desc;
}

void add_expected_value_int(test_harness_t* th, int64_t test_num, int64_t expected) {
	th->tests[test_num].test_type = TEST_TYPE_INT;
	th->tests[test_num].expected_val.int_val = expected;
}

void add_expected_value_double(test_harness_t* th, int64_t test_num, double expected) {
	th->tests[test_num].test_type = TEST_TYPE_DOUBLE;
	th->tests[test_num].expected_val.double_val = expected;
}

void add_expected_value_str(test_harness_t* th, int64_t test_num, char * expected) {
	th->tests[test_num].test_type = TEST_TYPE_STR;
	th->tests[test_num].expected_val.char_p_val = expected;
}

size_t test_expected_value_int(test_harness_t* th, int64_t test_num, int64_t test_val) {
	if (th->tests[test_num].test_type != TEST_TYPE_INT) {
		LOG_D_A("Test %lld:\n\t%s\n\tTest Value is Wrong Type\n",test_num, th->tests[test_num].desc);
		th->tests[test_num].passed_test = 0;
		return -1;
	}
	th->tests[test_num].test_val.int_val = test_val;
	if (test_val == th->tests[test_num].expected_val.int_val) {
		LOG_D_A("Test %lld:\n\t%s\n\tPASS\n",test_num, th->tests[test_num].desc);
		th->tests[test_num].passed_test = 1;
		return 1;
	} else {
		LOG_D_A("Test %lld:\n\t%s\n\tFAILED\n\tExpected: %lld - Observed: %lld\n",test_num, th->tests[test_num].desc, th->tests[test_num].expected_val.int_val,test_val);
		th->tests[test_num].passed_test = 0;
		return 0;
	}
}

size_t test_expected_value_double(test_harness_t* th, int64_t test_num, double test_val ) {
	if (th->tests[test_num].test_type != TEST_TYPE_DOUBLE) {
		LOG_D_A("Test %lld:\n\t%s\n\tTest Value is Wrong Type\n",test_num, th->tests[test_num].desc);
		th->tests[test_num].passed_test = 0;
		return -1;
	}
	th->tests[test_num].test_val.double_val = test_val;
	if (test_val == th->tests[test_num].expected_val.double_val) {
		LOG_D_A("Test %lld:\n\t%s\n\tPASS\n",test_num, th->tests[test_num].desc);
		th->tests[test_num].passed_test = 1;
		return 1;
	} else {
		LOG_D_A("Test %lld:\n\t%s\n\tFAILED\n\tExpected: %lf - Observed: %lf\n",test_num, th->tests[test_num].desc, th->tests[test_num].expected_val.double_val,test_val);
		th->tests[test_num].passed_test = 0;
		return 0;
	}
}

size_t test_expected_value_str(test_harness_t* th, int64_t test_num, char * test_val) {
	if (th->tests[test_num].test_type != TEST_TYPE_STR) {
		LOG_D_A("Test %lld:\n\t%s\n\tTest Value is Wrong Type\n",test_num, th->tests[test_num].desc);
		th->tests[test_num].passed_test = 0;
		return -1;
	}
	th->tests[test_num].test_val.char_p_val = test_val;
	if (!strcmp(test_val, th->tests[test_num].expected_val.char_p_val)) {
		LOG_D_A("Test %lld:\n\t%s\n\tPASS\n",test_num, th->tests[test_num].desc);
		th->tests[test_num].passed_test = 1;
		return 1;
	} else {
		LOG_D_A("Test %lld:\n\t%s\n\tFAILED\n\tExpected: %s - Observed: %s\n",test_num, th->tests[test_num].desc, th->tests[test_num].expected_val.char_p_val,test_val);
		th->tests[test_num].passed_test = 0;
		return 0;
	}
}

void fini_test_harness(test_harness_t* th) {
	int64_t num_passed_tests = 0;
	int64_t num_observed_tests = 0;

	for (int64_t i = 0; i < th->num_tests; i++) {
		if (th->tests[i].passed_test != -1) {
			num_observed_tests++;
			num_passed_tests += th->tests[i].passed_test;
		}
	}

	LOG_D_A("Passed %lld of %lld tests (Observed %lld tests)", num_passed_tests, th->num_tests, num_observed_tests);

	return;
}





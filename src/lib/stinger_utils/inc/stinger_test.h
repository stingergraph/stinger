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

enum TEST_TYPES {
	TEST_TYPE_INT,
	TEST_TYPE_DOUBLE,
	TEST_TYPE_STR
};

typedef union test_data_u {
	int64_t int_val;
	double double_val;
	char * char_p_val;
} test_data_t;

typedef struct test_st {
	char * desc;
	enum TEST_TYPES test_type;
	test_data_t expected_val;
	test_data_t test_val;
	size_t passed_test;
} test_t;

typedef struct test_harness_st {
	int64_t num_tests;
	test_t * tests;
} test_harness_t;

test_harness_t * new_test_harness(int64_t num_tests);

void add_test_description(test_harness_t* th, int64_t test_num, char * desc);

void add_expected_value_int(test_harness_t* th, int64_t test_num, int64_t expected);
void add_expected_value_double(test_harness_t* th, int64_t test_num, double expected);
void add_expected_value_str(test_harness_t* th, int64_t test_num, char * expected);

size_t test_expected_value_int(test_harness_t* th, int64_t test_num, int64_t test_val);
size_t test_expected_value_double(test_harness_t* th, int64_t test_num, double test_val );
size_t test_expected_value_str(test_harness_t* th, int64_t test_num, char * test_val);

void fini_test_harness(test_harness_t* th);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* STINGER_TEST_H_ */

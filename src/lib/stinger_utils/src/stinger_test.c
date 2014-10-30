/*
 * stinger_test.c
 *
 *  Created on: Oct 28, 2014
 *      Author: jason
 */

#define LOG_AT_E

#include "stinger_test.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"

test_harness_t * new_test_harness(int64_t num_tests) {
  test_harness_t * th = (test_harness_t *)xcalloc(1, sizeof(test_harness_t));
  th->tests = (test_t *)xcalloc(num_tests, sizeof(test_t));
  th->num_tests = num_tests;
  for (int64_t i=0; i < num_tests; i++) {
    th->tests[i].desc = NULL;
    th->tests[i].func = NULL;
    th->tests[i].func_arg = NULL;
    th->tests[i].passed_test = 1;
    th->tests[i].test_ran = 0;
  }
  return th;
}

size_t register_test(test_harness_t* th, char * desc, testfunc_t * func, void * func_arg) {
  if (th->next_test > th->num_tests) {
    return -1;
  }

  uint64_t test_num = th->next_test++;

  th->tests[test_num].desc = desc;
  th->tests[test_num].func = func;
  th->tests[test_num].func_arg = func_arg;

  return 1;
}

void run_tests(test_harness_t* th, FILE * output) {
  for (uint64_t i = 0; i < th->num_tests; i++) {
    if (th->tests[i].func != NULL) {
      run_test(th, i, output);
    }
  }
}

void run_test(test_harness_t* th, int64_t test_num, FILE * output) {
  testfunc_t * fptr = th->tests[test_num].func;
  if (fptr == NULL) {
    return;
  }

  th->current_test = test_num;
  th->tests[test_num].test_ran = 1;
  fptr(th, th->tests[test_num].func_arg);
  if (th->tests[test_num].passed_test) {
    fprintf(output, "Test %lld PASSED\n\n", test_num);
  } else {
    fprintf(output, "Test %lld FAILED\n\n", test_num);
  }
}

void print_test_summary(test_harness_t* th, FILE * output) {
  uint64_t passed_tests = 0;
  uint64_t ran_tests = 0;
  for (uint64_t i=0; i < th->num_tests; i++) {
    if (th->tests[i].test_ran) {
      ran_tests++;
    }
    if (th->tests[i].test_ran && th->tests[i].passed_test) {
      passed_tests++;
    }
  }

  fprintf(output, "%lld of %lld tests Passed (%lld registered)\n",passed_tests,ran_tests,th->next_test);
}






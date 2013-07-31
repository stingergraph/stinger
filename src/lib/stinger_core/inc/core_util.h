#ifndef STINGER_CORE_UTILS_H_
#define STINGER_CORE_UTILS_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include "stinger.h"

/* 
 * This file is intended to contain utility functions that are required for 
 * STINGER's core.  Any utility functions that STINGER core does not directly
 * depend on should be in the stinger_util library.
 */

int64_t bs64 (int64_t xin);

void bs64_n (size_t n, int64_t * restrict d);

int i64_cmp (const void *a, const void *b);

int i2cmp (const void *va, const void *vb);

int64_t prefix_sum (const int64_t n, int64_t *ary);

int64_t find_in_sorted (const int64_t tofind, const int64_t N, const int64_t * restrict ary);

int i64_cmp (const void *a, const void *b);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif

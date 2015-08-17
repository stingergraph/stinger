#define _FILE_OFFSET_BITS 64
#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include <errno.h>
#include <assert.h>

#include "compat.h"
#include "sorts.h"

/*
  All integer compares are in increasing order.
*/

MTA("mta expect parallel context")
void
insertion_iki (intvtx_t * restrict d, const size_t N)
{
  for (size_t i = 1; i < N; ++i) {
    const intvtx_t key = d[2*i];
    const intvtx_t weight = d[1+2*i];
    size_t k;
    for (k = i; k > 0 && d[2*(k-1)] > key; --k) {
      d[2*k] = d[2*(k-1)];
      d[1+2*k] = d[1+2*(k-1)];
    }
    if (i != k) {
      d[2*k] = key;
      d[1+2*k] = weight;
    }
  }
}

MTA("mta expect parallel context")
void
insertion_ikii (intvtx_t * restrict d, const size_t N)
{
  for (size_t i = 1; i < N; ++i) {
    const intvtx_t key = d[3*i];
    const intvtx_t t1 = d[1+3*i];
    const intvtx_t t2 = d[2+3*i];
    size_t k;
    for (k = i; k > 0 && d[3*(k-1)] > key; --k) {
      d[3*k] = d[3*(k-1)];
      d[1+3*k] = d[1+3*(k-1)];
      d[2+3*k] = d[2+3*(k-1)];
    }
    if (i != k) {
      d[3*k] = key;
      d[1+3*k] = t1;
      d[2+3*k] = t2;
    }
  }
}

MTA("mta expect parallel context")
void
shellsort_iki (intvtx_t * restrict d, const size_t N)
{
  /* Using Knuth's / Incerpi-Sedgewick's strides. */
  /* const size_t stride[] = { 1391376, 463792, 198768, 86961, 33936, */
  /*                                13776, 4592, 1968, 861, 336, */
  /*                                112, 48, 21, 7, 3, 1 }; */
  /* Using Marcin Ciura's strides. */
  const size_t stride[] = {701, 301, 132, 57, 23, 10, 4, 1};

  for (size_t k = 0; k < sizeof(stride)/sizeof(*stride); ++k) {
    const size_t h = stride[k];
    for (size_t i = h; i < N; ++i) {
      size_t j;
      intvtx_t t0 = d[2*i];
      intvtx_t t1 = d[1+2*i];
      for (j = i; j >= h && d[2*(j-h)] > t0; j -= h) {
        d[2*j] = d[2*(j-h)];
        d[1+2*j] = d[1+2*(j-h)];
      }
      if (j != i) {
        d[2*j] = t0;
        d[1+2*j] = t1;
      }
    }
  }
}

MTA("mta expect parallel context")
void
shellsort_ikii (intvtx_t * restrict d, const size_t N)
{
  /* Using Knuth's / Incerpi-Sedgewick's strides. */
  /* const size_t stride[] = { 1391376, 463792, 198768, 86961, 33936, */
  /*                                13776, 4592, 1968, 861, 336, */
  /*                                112, 48, 21, 7, 3, 1 }; */
  /* Using Marcin Ciura's strides. */
  const size_t stride[] = {701, 301, 132, 57, 23, 10, 4, 1};

  for (size_t k = 0; k < sizeof(stride)/sizeof(*stride); ++k) {
    const size_t h = stride[k];
    for (size_t i = h; i < N; ++i) {
      size_t j;
      intvtx_t t0 = d[3*i];
      intvtx_t t1 = d[1+3*i];
      intvtx_t t2 = d[1+3*i];
      for (j = i; j >= h && d[3*(j-h)] > t0; j -= h) {
        d[3*j] = d[3*(j-h)];
        d[1+3*j] = d[1+3*(j-h)];
        d[2+3*j] = d[2+3*(j-h)];
      }
      if (j != i) {
        d[3*j] = t0;
        d[1+3*j] = t1;
        d[1+3*j] = t2;
      }
    }
  }
}

#if !defined(SORT_THRESH)
#define SORT_THRESH 32
#endif

MTA("mta expect parallel context")
static inline void
thresh_shellsort_iki (intvtx_t * restrict d, const size_t N)
{
  /* Using Knuth's / Incerpi-Sedgewick's strides. */
  /* const size_t stride[] = { /\* 1391376, 463792, 198768, 86961, 33936, */
  /*                          13776, 4592, 1968, 861, 336, */
  /*                          112, 48, *\/ 21, 7, 3, 1 }; */
  /* Using Marcin Ciura's strides. */
  const size_t stride[] = { /* 701, 301, 132, 57, */ 23, 10, 4, 1};

  for (size_t k = 0; k < sizeof(stride)/sizeof(*stride); ++k) {
    const size_t h = stride[k];
    for (size_t i = h; i < N; ++i) {
      size_t j;
      intvtx_t t0 = d[2*i];
      intvtx_t t1 = d[1+2*i];
      for (j = i; j >= h && d[2*(j-h)] > t0; j -= h) {
        d[2*j] = d[2*(j-h)];
        d[1+2*j] = d[1+2*(j-h)];
      }
      if (j != i) {
        d[2*j] = t0;
        d[1+2*j] = t1;
      }
    }
  }
}

MTA("mta expect parallel context")
static inline void
thresh_shellsort_ikii (intvtx_t * restrict d, const size_t N)
{
  /* Using Knuth's / Incerpi-Sedgewick's strides. */
  /* const size_t stride[] = { /\* 1391376, 463792, 198768, 86961, 33936, */
  /*                          13776, 4592, 1968, 861, 336, */
  /*                          112, 48, *\/ 21, 7, 3, 1 }; */
  /* Using Marcin Ciura's strides. */
  const size_t stride[] = { /* 701, 301, 132, 57, */ 23, 10, 4, 1};

  for (size_t k = 0; k < sizeof(stride)/sizeof(*stride); ++k) {
    const size_t h = stride[k];
    for (size_t i = h; i < N; ++i) {
      size_t j;
      intvtx_t t0 = d[3*i];
      intvtx_t t1 = d[1+3*i];
      intvtx_t t2 = d[1+3*i];
      for (j = i; j >= h && d[3*(j-h)] > t0; j -= h) {
        d[3*j] = d[3*(j-h)];
        d[1+3*j] = d[1+3*(j-h)];
        d[2+3*j] = d[2+3*(j-h)];
      }
      if (j != i) {
        d[3*j] = t0;
        d[1+3*j] = t1;
        d[2+3*j] = t2;
      }
    }
  }
}

MTA("mta expect parallel context")
static intvtx_t
median_of_three (intvtx_t a, intvtx_t b, intvtx_t c)
{
  intvtx_t tmp;
  /* Insertion sort. */
  if (a > b) {
    tmp = a;
    a = b;
    b = tmp;
  }
  if (b > c) {
    tmp = b;
    b = c;
    c = tmp;
  }
  if (a > b) {
    tmp = a;
    a = b;
    b = tmp;
  }
  return b;
}

MTA("mta expect parallel context")
static int
floor_log2 (size_t x)
{
  if (!x) return 0;
#if defined(__GCC__)
#if SIZE_MAX < INT64_MAX
  return 31 - __builtin_clzl(x);
#else
  return 63 - __builtin_clzl(x);
#endif
#else
  int out = 0;
#if SIZE_MAX < INT64_MAX
  const uint32_t one = 1;
#else
  const uint64_t one = 1;
  if (x >= (one << 32)) { x >>= 32; out += 32; }
#endif
  if (x >= (one << 16)) { x >>= 16; out += 16; }
  if (x >= (one << 8)) { x >>=  8; out +=  8; }
  if (x >= (one << 4)) { x >>=  4; out +=  4; }
  if (x >= (one << 2)) { x >>=  2; out +=  2; }
  if (x >= (one << 1)) { out +=  1; }
  return out;
#endif
}

MTA("mta expect parallel context")
static intvtx_t
partition_iki (intvtx_t * restrict d, intvtx_t i, intvtx_t j,
               const intvtx_t piv)
{
  while (i <= j) {
    while (d[2*i] < piv)
      i++;
    while (d[2*j] > piv)
      j--;
    if (i <= j) {
      intvtx_t tmp0 = d[2*i];
      intvtx_t tmp1 = d[1+2*i];
      d[2*i] = d[2*j];
      d[1+2*i] = d[1+2*j];
      d[2*j] = tmp0;
      d[1+2*j] = tmp1;
      i++;
      j--;
    }
  };
  return j;
}

MTA("mta expect parallel context")
static size_t
partition_ikii (intvtx_t * restrict d, size_t i, size_t j,
                const intvtx_t piv)
{
  while (i <= j) {
    while (d[3*i] < piv)
      i++;
    while (d[3*j] > piv)
      j--;
    if (i <= j) {
      intvtx_t tmp0 = d[3*i];
      intvtx_t tmp1 = d[1+3*i];
      intvtx_t tmp2 = d[2+3*i];
      d[3*i] = d[3*j];
      d[1+3*i] = d[1+3*j];
      d[2+3*i] = d[2+3*j];
      d[3*j] = tmp0;
      d[1+3*j] = tmp1;
      d[2+3*j] = tmp2;
      i++;
      j--;
    }
  };
  return j;
}

MTA("mta expect parallel context")
static void
introsort_loop_iki (intvtx_t * restrict d, const size_t start, size_t end,
                    int depth_limit)
{
  while (end - start > SORT_THRESH) {
    if (!depth_limit) {
      thresh_shellsort_iki (&d[2*start], end-start); /* Yeah, should be heapsort... */
      return;
    }
    --depth_limit;
    const size_t p = partition_iki (d, start, end-1,
                                    median_of_three (d[2*start],
                                                     d[2*(start+(end-start)/2)],
                                                     d[2*(end-1)]));
    introsort_loop_iki (d, p, end, depth_limit);
    end = p;
  }
}

MTA("mta expect parallel context")
void
introsort_iki (intvtx_t * restrict d, const size_t N)
{
  if (N < 2) return;
  introsort_loop_iki (d, 0, N, 2*floor_log2 (N));
  insertion_iki (d, N);
}

MTA("mta expect parallel context")
static void
introsort_loop_ikii (intvtx_t * restrict d, const size_t start, size_t end,
                     int depth_limit)
{
  while (end - start > SORT_THRESH) {
    if (!depth_limit) {
      thresh_shellsort_iki (&d[3*start], end-start); /* Yeah, should be heapsort... */
      return;
    }
    --depth_limit;
    const size_t p = partition_ikii (d, start, end-1,
                                     median_of_three (d[3*start],
                                                      d[3*(start+(end-start)/2)],
                                                      d[3*(end-1)]));
    introsort_loop_ikii (d, p, end, depth_limit);
    end = p;
  }
}

MTA("mta expect parallel context")
void
introsort_ikii (intvtx_t * restrict d, const size_t N)
{
  if (N < 2) return;
  introsort_loop_ikii (d, 0, N, 2*floor_log2 (N));
  insertion_ikii (d, N);
}

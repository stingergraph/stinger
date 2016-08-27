#include <alloca.h>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include "core_util.h"
#include "stinger_defs.h"

#include "compat/getMemorySize.h"

/**
 * @brief Simple comparator for pairs of int64_t.
 *
 * integers in a pair must be stored contiguously.
 *
 * @param a Pointer to first integer pair.
 * @param b Pointer to second integer pair.
 * @return > 0 if a is greater, < 0 if b is greater, 0 if equal.
 */

int i2cmp (const void *va, const void *vb)
{
  const int64_t *a = va;
  const int64_t *b = vb;
  if (a[0] < b[0])
    return -1;
  if (a[0] > b[0])
    return 1;
  if (a[1] < b[1])
    return -1;
  if (a[1] > b[1])
    return 1;
  return 0;
}

 
int64_t
bs64 (int64_t)
#if defined(__GNUC__)
__attribute__ ((const))
#endif
;

int64_t
bs64 (int64_t xin)
{
  uint64_t x = (uint64_t) xin;  /* avoid sign-extension issues */
  x = (x >> 32) | (x << 32);
  x = ((x & ((uint64_t) 0xFFFF0000FFFF0000ull)) >> 16)
    | ((x & ((uint64_t) 0x0000FFFF0000FFFFull)) << 16);
  x = ((x & ((uint64_t) 0xFF00FF00FF00FF00ull)) >> 8)
    | ((x & ((uint64_t) 0x00FF00FF00FF00FFull)) << 8);
  return x;
}

void
bs64_n (size_t n, int64_t * restrict d)
{
  OMP ("omp parallel for")
    for (size_t k = 0; k < n; ++k)
      d[k] = bs64 (d[k]);
}

/** @brief Inclusive prefix sum utility function.
 *
 * This sum is inclusive meaning that the first element of the output will be
 * equal to the first element of the input (not necessarily 0).
 *
 * If compiled with OpenMP, assumes that you are in parallel section and are 
 * calling this with all threads (although it will also work outside of 
 * a parallel OpenMP context - just DO NOT call this in OpenMP single or master
 * inside of a parallel region).
 *
 * If compiled without OpenMP this is implemented as a simple serial prefix sum.
 *
 * @param n The input array size.
 * @param ary The input array pointer.
 * @return The final element of the sum.
 */
int64_t
prefix_sum (const int64_t n, int64_t *ary); /* re-declared here for doxygen */
#if defined(_OPENMP)
int64_t
prefix_sum (const int64_t n, int64_t *ary)
{
  static int64_t *buf;

  int nt, tid;
  int64_t slice_begin, slice_end, t1, k, tmp;

  nt = omp_get_num_threads ();
  tid = omp_get_thread_num ();

  OMP("omp master")
    buf = alloca (nt * sizeof (*buf));
  OMP("omp flush (buf)");
  OMP("omp barrier");

  slice_begin = (tid * n) / nt;
  slice_end = ((tid + 1) * n) / nt; 

  /* compute sums of slices */
  tmp = 0;
  for (k = slice_begin; k < slice_end; ++k)
    tmp += ary[k];
  buf[tid] = tmp;

  /* prefix sum slice sums */
  OMP("omp barrier");
  OMP("omp single")
    for (k = 1; k < nt; ++k)
      buf[k] += buf[k-1];
  OMP("omp barrier");

  /* get slice sum */
  if (tid)
    t1 = buf[tid-1];
  else
    t1 = 0;

  /* prefix sum our slice including previous slice sums */
  ary[slice_begin] += t1;
  for (k = slice_begin + 1; k < slice_end; ++k) {
    ary[k] += ary[k-1];
  }
  OMP("omp barrier");

  return ary[n-1];
}

#else

int64_t
prefix_sum (const int64_t n, int64_t *ary)
{
  for (int64_t k = 1; k < n; ++k) {
    ary[k] += ary[k-1];
  }
  return ary[n-1];
}
#endif

/**
 * @brief Find an element in a sorted array of int64_t.
 *
 * @param tofind The value.
 * @param N Array length.
 * @param ary Array pointer.
 * @return Index in the array or -1 if not found.
 */
 
int64_t
find_in_sorted (const int64_t tofind,
                const int64_t N, const int64_t * restrict ary)
{
  int64_t out = -1;
  if (N <= 0)
    return -1;

  int64_t top = N - 1, bot = 0;

  if (tofind == ary[bot])
    return bot;
  if (tofind == ary[top])
    return top;
  while (top - bot + 1 > 4) {
    int64_t mid = bot + ((top - bot) / 2);
    if (tofind == ary[mid])
      return mid;
    if (tofind < ary[mid])
      top = mid;
    else
      bot = mid;
  }
  for (; bot <= top; ++bot)
    if (tofind == ary[bot])
      return bot;
  return -1;
}

/**
 * @brief Simple comparator for int64_t.
 *
 * @param a Pointer to first integer.
 * @param b Pointer to second integer.
 * @return > 0 if a is greater, < 0 if b is greater, 0 if equal.
 */
int
i64_cmp (const void *a, const void *b)
{
  const int64_t va = *(const int64_t *) a;
  const int64_t vb = *(const int64_t *) b;
  return va - vb;
}

static size_t max_memsize_env = 0;
static void
set_max_memsize_env (void)
{
  if (max_memsize_env != 0) return;
  if (getenv ("STINGER_MAX_MEMSIZE")) {
    char *tailptr = NULL;
    unsigned long mx;
    errno = 0;
    mx = strtoul (getenv ("STINGER_MAX_MEMSIZE"), &tailptr, 10);
    if (ULONG_MAX != mx && errno == 0) {
      if (tailptr)
        switch (*tailptr) {
        case 't':
        case 'T':
          mx <<= 10;
        case 'g':
        case 'G':
          mx <<= 10;
        case 'm':
        case 'M':
          mx <<= 10;
        case 'k':
        case 'K':
          mx <<= 10;
          break;
        }
    }
    max_memsize_env = mx;
  } else {
    max_memsize_env = getMemorySize() / 2; // Default to 1/2 memory size
  }
}

size_t
stinger_max_memsize (void)
{
  size_t out;
  set_max_memsize_env ();
  out = max_memsize_env;
  return out;
}


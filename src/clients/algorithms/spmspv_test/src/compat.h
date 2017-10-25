#if !defined(COMPAT_HEADER_local_)
#define COMPAT_HEADER_local_

#include "stinger_core/stinger.h"

#if defined(__GNUC__)
#define FN_MAY_BE_UNUSED __attribute__((unused))
#else
#define FN_MAY_BE_UNUSED
#endif

static inline int64_t
int64_fetch_add (int64_t * p, int64_t incr) FN_MAY_BE_UNUSED;
static inline int32_t
int32_fetch_add (int32_t * p, int32_t incr) FN_MAY_BE_UNUSED;

int64_t
int64_fetch_add (int64_t * p, int64_t incr)
{
#if defined(_OPENMP)
#if defined(__GNUC__)
  return __sync_fetch_and_add (p, incr);
#elif defined(__INTEL_COMPILER)
  return __sync_fetch_and_add (p, incr);
#else
#error "Atomics not defined..."
#endif
#else
  int64_t out = *p;
  *p += incr;
  return out;
#endif
}

int32_t
int32_fetch_add (int32_t * p, int32_t incr)
{
#if defined(_OPENMP)
#if defined(__GNUC__)
  return __sync_fetch_and_add (p, incr);
#elif defined(__INTEL_COMPILER)
  return __sync_fetch_and_add (p, incr);
#else
#error "Atomics not defined..."
#endif
#else
  int32_t out = *p;
  *p += incr;
  return out;
#endif
}

static inline int64_t
int64_compare_and_swap (int64_t * p, int64_t oldval, int64_t newval)
  FN_MAY_BE_UNUSED;

static inline int
bool_int64_compare_and_swap (int64_t * p, int64_t oldval, int64_t newval)
  FN_MAY_BE_UNUSED;

int64_t
int64_compare_and_swap (int64_t * p, int64_t oldval, int64_t newval)
{
#if defined(_OPENMP)&&(defined(__GNUC__)||defined(__INTEL_COMPILER))
  return __sync_val_compare_and_swap (p, oldval, newval);
#elif defined(_OPENMP)
#error "Atomics not defined..."
#else
  int64_t t = *p;
  if (t == oldval) *p = newval;
  return t;
#endif
}

int
bool_int64_compare_and_swap (int64_t * p, int64_t oldval, int64_t newval)
{
#if defined(_OPENMP)&&(defined(__GNUC__)||defined(__INTEL_COMPILER))
  return __sync_bool_compare_and_swap (p, oldval, newval);
#elif defined(_OPENMP)
#error "Atomics not defined..."
#else
  int64_t t = *p;
  if (t == oldval) *p = newval;
  return t == oldval;
#endif
}

#endif /* COMPAT_HEADER_local_ */

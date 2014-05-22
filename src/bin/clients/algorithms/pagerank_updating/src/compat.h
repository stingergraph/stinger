#if !defined(COMPAT_HEADER_local_)
#define COMPAT_HEADER_local_

#if defined(__MTA__)
#if !defined(MTA)
#define MTA(x) _Pragma(x)
#endif
#if !defined(NDEBUG)
#define MTA_NODEP MTA("mta assert parallel")
#else
#define MTA_NODEP MTA("mta assert nodep")
#define nonMTA_break
#endif
#else
#if !defined(MTA)
#define MTA(x)
#endif
#define MTA_NODEP
#define nonMTA_break break
#endif

#if defined(_OPENMP)
#include <omp.h>
#if !defined(OMP)
#define OMP(x) OMP_(x)
#define OMP_(x) _Pragma(x)
#endif
#else
#if !defined(OMP)
#define OMP(x)
#endif
static inline int omp_get_num_threads (void) { return 1; }
static inline int omp_get_thread_num (void) { return 0; }
#endif

#if defined(__GNUC__)
#define FN_MAY_BE_UNUSED __attribute__((unused))
#else
#define FN_MAY_BE_UNUSED
#endif

static inline int64_t
int64_fetch_add (int64_t * p, int64_t incr) FN_MAY_BE_UNUSED;
static inline int32_t
int32_fetch_add (int32_t * p, int32_t incr) FN_MAY_BE_UNUSED;

MTA("mta inline")
MTA("mta expect parallel")
int64_t
int64_fetch_add (int64_t * p, int64_t incr)
{
#if defined(__MTA__)
  return int_fetch_add (p, incr);
#elif defined(_OPENMP)
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

MTA("mta inline")
MTA("mta expect parallel")
int32_t
int32_fetch_add (int32_t * p, int32_t incr)
{
#if defined(__MTA__)
  /* Will warn about being too small... */
  return int_fetch_add (p, incr);
#elif defined(_OPENMP)
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

MTA("mta inline")
MTA("mta expect parallel")
int64_t
int64_compare_and_swap (int64_t * p, int64_t oldval, int64_t newval)
{
#if defined(__MTA__)
  int64_t t = *p;
  if (t == oldval) {
    int64_t t;
    t = readfe (p);
    if (t == oldval) {
      writeef (p, newval);
    } else {
      writeef (p, oldval);
    }
  }
  return t;
#elif defined(_OPENMP)&&(defined(__GNUC__)||defined(__INTEL_COMPILER))
  return __sync_val_compare_and_swap (p, oldval, newval);
#elif defined(_OPENMP)
#error "Atomics not defined..."
#else
  int64_t t = *p;
  if (t == oldval) *p = newval;
  return t;
#endif
}

MTA("mta inline")
MTA("mta expect parallel")
int
bool_int64_compare_and_swap (int64_t * p, int64_t oldval, int64_t newval)
{
#if defined(__MTA__)
  if (*p == oldval) {
    int64_t t;
    t = readfe (p);
    if (t == oldval)
      t = newval;
    writeef (p, t);
  }
  return t == oldval;
#elif defined(_OPENMP)&&(defined(__GNUC__)||defined(__INTEL_COMPILER))
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

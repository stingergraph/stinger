#if !defined(COMPAT_HEADER_)
#define COMPAT_HEADER_

#include "stinger_core/stinger.h"
#include "defs.h"

#if defined(__GNUC__)
#define FN_MAY_BE_UNUSED __attribute__((unused))
#else
#define FN_MAY_BE_UNUSED
#endif

static inline int64_t
int64_fetch_add (int64_t * p, int64_t incr) FN_MAY_BE_UNUSED;
static inline int32_t
int32_fetch_add (int32_t * p, int32_t incr) FN_MAY_BE_UNUSED;

#if defined(USE32BIT)
#define intvtx_fetch_add int32_fetch_add
#else
#define intvtx_fetch_add int64_fetch_add
#endif



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

static inline void purge (void *)
  FN_MAY_BE_UNUSED;
void
purge (void *p64)
{
  *(int64_t*)p64 = 0;
}
inline static int64_t writeef (int64_t *, int64_t)
  FN_MAY_BE_UNUSED;
int64_t
writeef (int64_t *p, int64_t v)
{
  return (*p = v);
}
inline static int64_t readfe (int64_t *)
  FN_MAY_BE_UNUSED;
int64_t
readfe (int64_t *p)
{
  return *p;
}
inline static int64_t readff (int64_t *)
  FN_MAY_BE_UNUSED;
int64_t
readff (int64_t *p)
{
  return *p;
}
inline static double dreadfe (double *)
  FN_MAY_BE_UNUSED;
double
dreadfe (double *p)
{
  return *p;
}
inline static double dwriteef (double *, double)
  FN_MAY_BE_UNUSED;
double
dwriteef (double *p, double v)
{
  return (*p = v);
}

static inline int64_t take_i64 (int64_t*) FN_MAY_BE_UNUSED;
static inline void release_i64 (int64_t*, int64_t) FN_MAY_BE_UNUSED;
static inline void take_i64_pair (int64_t*, int64_t*, int64_t*, int64_t*) FN_MAY_BE_UNUSED;
static inline void release_i64_pair (int64_t*, int64_t, int64_t*, int64_t) FN_MAY_BE_UNUSED;
#if defined(_OPENMP)
void
take_i64_pair (int64_t *p1, int64_t *v1, int64_t *p2, int64_t *v2)
{
  int took = 0;
  int64_t out1, out2;
  do {
    OMP("omp critical (TAKE)") {
      out1 = *p1;
      out2 = *p2;
      if (out1 != -1 && out2 != -1) {
       *p1 = -1;
       *p2 = -1;
       OMP("omp flush (p1, p2)");
       took = 1;
      }
    }
  } while (!took);
  *v1 = out1;
  *v2 = out2;
}
void
release_i64_pair (int64_t *p1, int64_t v1, int64_t *p2, int64_t v2)
{
  assert (*p1 == -1);
  assert (*p2 == -1);
  OMP("omp critical (TAKE)") {
    *p1 = v1;
    *p2 = v2;
    OMP("omp flush (p1, p2)");
  }
  return;
}
#if defined(__GNUC__)||defined(__INTEL_COMPILER)
int64_t
take_i64 (int64_t *p)
{
  int64_t oldval;

  do {
    oldval = *p;
  } while (!(oldval != -1 && __sync_bool_compare_and_swap (p, oldval, -1)));
  return oldval;
}
void
release_i64 (int64_t *p, int64_t val)
{
  assert (*p == -1);
  *p = val;
}
#else
/* XXX: These suffice for now. */
int64_t
take_i64 (int64_t *p)
{
  int64_t out;
  do {
    OMP("omp critical (TAKE)") {
      out = *p;
      if (out != -1)
       *p = -1;
    }
  } while (out < 0);
  OMP("omp flush (p)");
  return out;
}
void
release_i64 (int64_t *p, int64_t val)
{
  assert (*p == -1);
  OMP("omp critical (TAKE)") {
    *p = val;
    OMP("omp flush (p)");
  }
  return;
}
#endif

#else
int64_t
take_i64 (int64_t *p)
{
  int64_t out = *p;
  *p = -1;
  return out;
}
void
release_i64 (int64_t *p, int64_t val)
{
  assert (*p == -1);
  *p = val;
}
#endif

#endif /* COMPAT_HEADER_ */

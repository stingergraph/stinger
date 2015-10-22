/* -*- mode: C; mode: folding; fill-column: 70; -*- */
#if !defined(STINGER_ATOMICS_H_)
#define STINGER_ATOMICS_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include "stinger.h"

static inline int stinger_int_fetch_add (int *, int);
static inline int64_t stinger_int64_fetch_add (int64_t *, int64_t);
static inline uint64_t stinger_uint64_fetch_add (uint64_t *, uint64_t);
static inline size_t stinger_size_fetch_add (size_t *, size_t);

static inline void stinger_int64_swap (int64_t *, int64_t *);
static inline void stinger_uint64_swap (uint64_t *, uint64_t *);

static inline int64_t stinger_int64_cas (int64_t *, int64_t, int64_t);
/* XXX: This declaration may break aliasing, depending on how you
   interpret "void".  A pointer to nothing cannot alias anything...
   however, the absolutely correct unsigned char* is a pain to use,
   and people huddle and cry when they see caddr_t. */
static inline void *stinger_ptr_cas (void **, void *, void *);

#if defined(__MTA__)
/* {{{ MTA defs */
#pragma mta inline
int
stinger_int_fetch_add (int *x, int i)
{
  return int_fetch_add (x, i);
}

#pragma mta inline
int64_t
stinger_int64_fetch_add (int64_t * x, int64_t i)
{
  return int_fetch_add (x, i);
}

#pragma mta inline
uint64_t
stinger_uint64_fetch_add (uint64_t * x, uint64_t i)
{
  return int_fetch_add (x, i);
}

#pragma mta inline
size_t
stinger_size_fetch_add (size_t * x, size_t i)
{
  return int_fetch_add (x, i);
}

#pragma mta inline
void
stinger_int64_swap (int64_t * x, int64_t * y)
{
  int64_t vx, vy, t;
  vx = readfe (x);
  vy = readfe (y);
  writeef (x, vy);
  writeef (y, vx);
}

#pragma mta inline
void
stinger_uint64_swap (uint64_t * x, uint64_t * y)
{
  uint64_t vx, vy, t;
  vx = readfe (x);
  vy = readfe (y);
  writeef (x, vy);
  writeef (y, vx);
}

#pragma mta inline
void *
stinger_ptr_cas (void **x, void *origx, void *newx)
{
  void *curx, *updx;
  curx = readfe (x);
  if (curx == origx)
    updx = newx;
  else
    updx = curx;
  writeef (x, updx);
  return curx;
}

#pragma mta inline
int64_t
stinger_int64_cas (int64_t * x, int64_t origx, int64_t newx)
{
  int64_t curx, updx;
  curx = readfe (x);
  if (curx == origx)
    updx = newx;
  else
    updx = curx;
  writeef (x, updx);
  return curx;
}

/* }}} */
#elif defined(__GNUC__)||defined(__INTEL_COMPILER)
/* {{{ GCC / ICC defs */
int
stinger_int_fetch_add (int *x, int i)
{
  return __sync_fetch_and_add (x, i);
}

int64_t
stinger_int64_fetch_add (int64_t * x, int64_t i)
{
  return __sync_fetch_and_add (x, i);
}

uint64_t
stinger_uint64_fetch_add (uint64_t * x, uint64_t i)
{
  return __sync_fetch_and_add (x, i);
}

size_t
stinger_size_fetch_add (size_t * x, size_t i)
{
  return __sync_fetch_and_add (x, i);
}

void
stinger_int64_swap (int64_t * x, int64_t * y)
{
  int64_t vx;
  do
    {
      vx = *x;
    }
  while (!__sync_bool_compare_and_swap (x, vx, *y));
}

void
stinger_uint64_swap (uint64_t * x, uint64_t * y)
{
  uint64_t vx;
  do
    {
      vx = *x;
    }
  while (!__sync_bool_compare_and_swap (x, vx, *y));
}

void *
stinger_ptr_cas (void **x, void *origx, void *newx)
{
  return __sync_val_compare_and_swap (x, origx, newx);
}

int64_t
stinger_int64_cas (int64_t * x, int64_t origx, int64_t newx)
{
  return __sync_val_compare_and_swap (x, origx, newx);
}

/* }}} */
#elif defined(__xlc__)
/* {{{ XLC defs */
int
stinger_int_fetch_add (int *x, int i)
{
  return __fetch_and_add (x, i);
}

int64_t
stinger_int64_fetch_add (int64_t * x, int64_t i)
{
  return __fetch_and_addlp ((volatile unsigned long*)x, (unsigned long)i);
}

uint64_t
stinger_uint64_fetch_add (uint64_t * x, uint64_t i)
{
  return __fetch_and_addlp ((int64_t*)x, i);
}

size_t
stinger_size_fetch_add (size_t * x, size_t i)
{
  return __fetch_and_addlp ((int64_t*)x, i);
}

void
stinger_int64_swap (int64_t * x, int64_t * y)
{
  int64_t vx;
  do
    {
      vx = *x;
    }
  while (!__compare_and_swaplp (x, &vx, *y));
}

void
stinger_uint64_swap (uint64_t * x, uint64_t * y)
{
  uint64_t vx;
  do
    {
      vx = *x;
    }
  while (!__compare_and_swaplp ((int64_t*)x, (int64_t*)&vx, *y));
}

void *
stinger_ptr_cas (void **x, void *origx, void *newx)
{
  void * t = *x;
  __compare_and_swaplp ((int64_t*)x, (int64_t*)&origx, (int64_t)newx);
  return t;
}

int64_t
stinger_int64_cas (int64_t * x, int64_t origx, int64_t newx)
{
  int64_t t = *x;
  __compare_and_swaplp (x, &origx, newx);
  return t;
}

/* }}} */
#elif !defined(_OPENMP)
#warning "Assuming no parallel / concurrent operations necessary."
/* {{{ Not concurrent */
int
stinger_int_fetch_add (int *v, int x)
{
  int out = *v;
  *v += x;
  return out;
}

int64_t
stinger_int64_fetch_add (int64_t * v, int64_t x)
{
  int64_t out = *v;
  *v += x;
  return out;
}

uint64_t
stinger_uint64_fetch_add (uint64_t * v, uint64_t x)
{
  uint64_t out = *v;
  *v += x;
  return out;
}

size_t
stinger_size_fetch_add (size_t * v, size_t x)
{
  size_t out = *v;
  *v += x;
  return out;
}

void
stinger_int64_swap (int64_t * a, int64_t * b)
{
  int64_t t = *a;
  *a = *b;
  *b = t;
}

void
stinger_uint64_swap (uint64_t * a, uint64_t * b)
{
  uint64_t t = *a;
  *a = *b;
  *b = t;
}

int64_t
stinger_int64_cas (int64_t * p, int64_t v, int64_t newv)
{
  int64_t oldv = *p;
  if (v == oldv)
    *p = newv;
  return oldv;
}

void *
stinger_ptr_cas (void **p, void *v, void *newv)
{
  void *oldv = *p;
  if (v == oldv)
    *p = newv;
  return oldv;
}

/* }}} */
#else
#error "Needs atomic operations defined."
#endif

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* STINGER_ATOMICS_H_ */

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <assert.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/stinger_error.h"
#include "stinger_core/xmalloc.h"

#include "spmv_spmspv.h"

#if defined(__GNUC__)
#if __GNUC__ < 4
#error "Ancient gcc needs updating."
#elif __GNUC__ == 4 && __GNUC_MINOR__ < 7
/* Define __atomic in terms of __sync */
#define __atomic_compare_exchange(p,pv,newpv,unused1,unused2,unused3) __sync_bool_compare_and_swap((p), *(pv), *(newpv))
#define __atomic_compare_exchange_n(p,pv,newv,unused1,unused2,unused3) __sync_bool_compare_and_swap((p), *(pv), (newv))
#define __atomic_load(p, pv, unused1) (pv) = *(p)
#define __atomic_store(p, pv, unused1) *(p) = *(pv)
#define __atomic_fetch_add(p, pv, unused1) __sync_fetch_and_add((p), (pv))
#endif
#else
#error "Needs atomic support, preferably through C11-ish..."
#endif

#if defined(__GNUC__)
#define FN_MAY_BE_UNUSED __attribute__((unused))
#else
#define FN_MAY_BE_UNUSED
#endif

//const static double NAN_EMPTY = nan ("0xDEADFACE");
#define NAN_EMPTY nan ("0xDEADFACE")  // because clang doesn't like the alternative
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define MM_PAUSE() _mm_pause()
#else
#define MM_PAUSE() do { } while (0)
#endif

#if !defined(ATOMIC_FP_FE_EMUL) && !defined(ATOMIC_FP_OPTIMISTIC) && !defined(ATOMIC_FP_OMP)
/* #define ATOMIC_FP_OMP */
/* #define ATOMIC_FP_FE_EMUL */
#define ATOMIC_FP_OPTIMISTIC
#endif

// HACK enable alternate OMP macro
#include "stinger_core/alternative_omp_macros.h"

static inline void
atomic_daccum (double *p, const double val)
{
#if defined(ATOMIC_FP_FE_EMUL)
  double pv, upd;
  int done = 0;
  do {
    __atomic_load ((int64_t*)p, (int64_t*)&pv, __ATOMIC_ACQUIRE);
    if (__atomic_compare_exchange ((int64_t*)p, (int64_t*)&pv, (int64_t*)&NAN_EMPTY, 1, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
      upd = pv + val;
      __atomic_store ((int64_t*)p, (int64_t*)&upd, __ATOMIC_RELEASE);
      done = 1;
    } else
      MM_PAUSE();
  } while (!done);
#elif defined(ATOMIC_FP_OPTIMISTIC)
  double pv, upd;
  __atomic_load ((int64_t*)p, (int64_t*)&pv, __ATOMIC_ACQUIRE);
  do {
    upd = pv + val;
    if (__atomic_compare_exchange ((int64_t*)p, (int64_t*)&pv, (int64_t*)&upd, 1, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
      break;
    else
      MM_PAUSE();
  } while (1);
#else
  OMP(omp atomic) *p += val;
#endif
}

static inline void setup_y (const int64_t nv, const double beta, double * y)
{
  if (0.0 == beta) {
    OMP(omp for OMP_SIMD)
      for (int64_t i = 0; i < nv; ++i)
        y[i] = 0.0;
  } else if (-1.0 == beta) {
    OMP(omp for OMP_SIMD)
      for (int64_t i = 0; i < nv; ++i)
        y[i] = -y[i];
  } else if (!(1.0 == beta)) {
    OMP(omp for OMP_SIMD)
      for (int64_t i = 0; i < nv; ++i)
        y[i] *= beta;
  }
  /* if 1.0 == beta, do nothing. */
}

/* Fragile macro, do not pass unevaluated expressions as arguments. */
/* It's a macro to avoid a sequence point that could force fetching x[i]... */
#define ALPHAXI_VAL(alpha, xi) (alpha == 0.0? 0.0 : (alpha == 1.0? xi : (alpha == -1.0? -xi : (alpha * xi))))
#define DEGSCALE(axi, degi) (degi == 0? 0.0 : axi / degi);

static inline void
dspmTv_accum (const struct stinger * S, const int64_t i, const double alphaxi, double * y)
{
  STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
    const int64_t j = STINGER_RO_EDGE_DEST;
    if (i != j) {
         const double aij = STINGER_RO_EDGE_WEIGHT;
      atomic_daccum (&y[j], aij * alphaxi);
    }
  } STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END();
}

static inline void
dspmTv_unit_accum (const struct stinger * S, const int64_t i, const double alphaxi, double * y)
{
  STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
    const int64_t j = STINGER_RO_EDGE_DEST;
    if (i != j) atomic_daccum (&y[j], alphaxi);
  } STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END();
}

void stinger_dspmTv (const int64_t nv, const double alpha, const struct stinger *S, const double * x, const double beta, double * y)
{
  OMP(omp parallel) {
    setup_y (nv, beta, y);

    OMP(omp for)
      for (int64_t i = 0; i < nv; ++i) {
        const double alphaxi = ALPHAXI_VAL (alpha, x[i]);
        if (alphaxi != 0.0)
          dspmTv_accum (S, i, alphaxi, y);
      }
  }
}

void stinger_unit_dspmTv (const int64_t nv, const double alpha, const struct stinger *S, const double * x, const double beta, double * y)
{
  OMP(omp parallel) {
    setup_y (nv, beta, y);

    OMP(omp for)
      for (int64_t i = 0; i < nv; ++i) {
        const double alphaxi = ALPHAXI_VAL (alpha, x[i]);
        if (alphaxi != 0.0)
          dspmTv_unit_accum (S, i, alphaxi, y);
      }
  }
}

void stinger_dspmTv_degscaled (const int64_t nv, const double alpha, const struct stinger *S, const double * x, const double beta, double * y)
{
  OMP(omp parallel) {
    setup_y (nv, beta, y);

    OMP(omp for)
      for (int64_t i = 0; i < nv; ++i) {
        const double alphaxi = ALPHAXI_VAL (alpha, x[i]);
        if (alphaxi != 0.0) {
          const int64_t degi = stinger_outdegree_get (S, i);
          const double alphaxi_deg = DEGSCALE (alphaxi, degi);
          dspmTv_accum (S, i, alphaxi_deg, y);
        }
      }
  }
}

void stinger_unit_dspmTv_degscaled (const int64_t nv, const double alpha, const struct stinger *S, const double * x, const double beta, double * y)
{
  //OMP(omp parallel)
  {
    setup_y (nv, beta, y);

    OMP(omp for)
      for (int64_t i = 0; i < nv; ++i) {
        const double alphaxi = ALPHAXI_VAL (alpha, x[i]);
        const int64_t degi = stinger_outdegree_get (S, i);
        if (alphaxi != 0.0 && degi > 0) {
          const double alphaxi_deg = DEGSCALE (alphaxi, degi);
          dspmTv_unit_accum (S, i, alphaxi_deg, y);
        }
      }
  }
}

static void setup_workspace (const int64_t nv, int64_t * restrict * loc_ws, double * restrict * val_ws)
{
  if (!*loc_ws) {
    abort ();
    OMP(omp master) {
      *loc_ws = xmalloc (nv * sizeof (**loc_ws));
    }
    OMP(omp barrier);
    OMP(omp for OMP_SIMD)
      for (int64_t k = 0; k < nv; ++k) (*loc_ws)[k] = -1;
  }
  if (!*val_ws) {
    abort ();
    OMP(omp master) {
      *val_ws = xmalloc (nv * sizeof (**val_ws));
    }
    OMP(omp barrier);
    OMP(omp for OMP_SIMD)
      for (int64_t k = 0; k < nv; ++k) (*val_ws)[k] = 0.0;
  }
}

static void setup_sparse_y (const double beta,
                            int64_t y_deg, int64_t * y_idx, double * y_val,
                            int64_t * loc_ws, double * val_ws)
{
  if (0.0 == beta) {
    OMP(omp for OMP_SIMD)
      for (int64_t k = 0; k < y_deg; ++k) {
        const int64_t i = y_idx[k];
        loc_ws[i] = k;
        val_ws[i] = 0.0;
      }
  } else if (1.0 == beta) {
    /* Still have to set up the pattern... */
    OMP(omp for OMP_SIMD)
      for (int64_t k = 0; k < y_deg; ++k) {
        const int64_t i = y_idx[k];
        loc_ws[i] = k;
        val_ws[i] = y_val[k];
      }
  } else if (-1.0 == beta) {
    OMP(omp for OMP_SIMD)
      for (int64_t k = 0; k < y_deg; ++k) {
        const int64_t i = y_idx[k];
        loc_ws[i] = k;
        val_ws[i] = -y_val[k];
      }
  } else if (1.0 != beta) {
    OMP(omp for OMP_SIMD)
      for (int64_t k = 0; k < y_deg; ++k) {
        const int64_t i = y_idx[k];
        const double yi = y_val[k];
        loc_ws[i] = k;
        val_ws[i] = beta * y_val[k];
      }
  }
}

/* Want this in L1... */
#define BATCH_SIZE 128
struct batch {
     int64_t n;
     int64_t idx[BATCH_SIZE];
};

#define BATCH_INIT {.n = 0}

static void
flush (struct batch *b, int64_t * y_deg, int64_t * y_idx, int64_t * loc_ws)
{
     const int64_t n = b->n;
     int64_t * restrict idx = b->idx;
     int64_t successful = 0;

     for (int64_t k = 0; k < n; ++k) {
          const int64_t j = idx[k];
          int64_t expected = -1;
          int64_t desired = -2;
          if (__atomic_compare_exchange_n (&loc_ws[j], &expected, &desired, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
               ++successful; /* Got it... */
          else
               idx[k] = -1; /* Someone else got it... */
     }
     if (successful) {
          int64_t where = __atomic_fetch_add (y_deg, successful, __ATOMIC_RELAXED);
          const int64_t where_end = where + successful;
          for (int64_t k = 0; k < n && where < where_end; ++k) {
               const int64_t j = idx[k];
               if (j >= 0) {
                    __atomic_store (&loc_ws[j], &where, __ATOMIC_RELEASE);
                    y_idx[where] = j;
                    ++where;
               }
          }
     }
     b->n = 0;
}

static inline void
enqueue (struct batch * b, const int64_t j, int64_t * y_deg, int64_t * y_idx, int64_t * loc_ws)
{
     assert(b->n <= BATCH_SIZE);
     if (b->n == BATCH_SIZE) {
          flush (b, y_deg, y_idx, loc_ws);
          assert(b->n == 0);
     }
     b->idx[b->n++] = j;
}

static inline void
dspmTspv_accum (const struct stinger * S, const int64_t i, const double alphaxi,
                int64_t * y_deg, int64_t * y_idx, double * y,
                int64_t * loc_ws, struct batch *b)
{
  STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
    const int64_t j = STINGER_RO_EDGE_DEST;
    if (i != j) {
      const double aij = STINGER_RO_EDGE_WEIGHT;
      atomic_daccum (&y[j], aij * alphaxi);
      if (loc_ws[j] < 0) enqueue (b, j, y_deg, y_idx, loc_ws);
    }
  } STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END();
}

static inline void
dspmTspv_unit_accum (const struct stinger * S, const int64_t i, const double alphaxi,
                     int64_t * y_deg, int64_t * y_idx, double * y,
                     int64_t * loc_ws, struct batch *b)
{
  STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
    const int64_t j = STINGER_RO_EDGE_DEST;
    if (i != j) {
      atomic_daccum (&y[j], alphaxi);
      if (loc_ws[j] < 0) enqueue (b, j, y_deg, y_idx, loc_ws);
    }
  } STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END();
}

static inline void
pack_vals (const int64_t y_deg, const int64_t * restrict y_idx,
           double * restrict val_ws, double * restrict y_val)
{
  /* Pack the values back into the shorter form. */
  OMP(omp for OMP_SIMD)
    for (int64_t k = 0; k < y_deg; ++k) {
      y_val[k] = val_ws[y_idx[k]];
      val_ws[y_idx[k]] = 0.0;
    }
}

void stinger_dspmTspv (const int64_t nv, const double alpha, const struct stinger *S, const int64_t x_deg, const int64_t * x_idx, const double * x_val, const double beta, int64_t * y_deg_ptr, int64_t * y_idx, double * y_val, int64_t * loc_ws_in, double * val_ws_in)
{
  int64_t * loc_ws = loc_ws_in;
  double * val_ws = val_ws_in;

  OMP(omp parallel shared(loc_ws, val_ws)) {
    setup_workspace (nv, &loc_ws, &val_ws);
    setup_sparse_y (beta, *y_deg_ptr, y_idx, y_val, loc_ws, val_ws);
    struct batch b = BATCH_INIT;

    OMP(omp for nowait)
      for (int64_t xk = 0; xk < x_deg; ++xk) {
        const int64_t i = x_idx[xk];
        const double alphaxi = ALPHAXI_VAL (alpha, x_val[xk]);
        if (alphaxi != 0.0)
             dspmTspv_accum (S, i, alphaxi, y_deg_ptr, y_idx, val_ws, loc_ws, &b);
        assert(b.n <= BATCH_SIZE);
      }
    flush (&b, y_deg_ptr, y_idx, loc_ws);
    assert(b.n == 0);
    OMP(omp barrier);

    pack_vals (*y_deg_ptr, y_idx, val_ws, y_val);
  }

  OMP(omp barrier);
  OMP(omp master) {
       if (!val_ws_in) free (val_ws);
       if (!loc_ws_in) free (loc_ws);
  }
}

void stinger_unit_dspmTspv (const int64_t nv, const double alpha, const struct stinger *S, const int64_t x_deg, const int64_t * x_idx, const double * x_val, const double beta, int64_t * y_deg_ptr, int64_t * y_idx, double * y_val, int64_t * loc_ws_in, double * val_ws_in)
{
  int64_t * loc_ws = loc_ws_in;
  double * val_ws = val_ws_in;

  OMP(omp parallel shared(loc_ws, val_ws)) {
    setup_workspace (nv, &loc_ws, &val_ws);
    setup_sparse_y (beta, *y_deg_ptr, y_idx, y_val, loc_ws, val_ws);
    struct batch b = BATCH_INIT;

    OMP(omp for nowait)
      for (int64_t xk = 0; xk < x_deg; ++xk) {
        const int64_t i = x_idx[xk];
        const double alphaxi = ALPHAXI_VAL (alpha, x_val[xk]);
        if (alphaxi != 0.0)
             dspmTspv_unit_accum (S, i, alphaxi, y_deg_ptr, y_idx, val_ws, loc_ws, &b);
      }
    flush (&b, y_deg_ptr, y_idx, loc_ws);
    OMP(omp barrier)

    pack_vals (*y_deg_ptr, y_idx, val_ws, y_val);
  }

  OMP(omp barrier);
  OMP(omp master) {
       if (!val_ws_in) free (val_ws);
       if (!loc_ws_in) free (loc_ws);
  }
}

void stinger_dspmTspv_degscaled (const int64_t nv, const double alpha, const struct stinger *S, const int64_t x_deg, const int64_t * x_idx, const double * x_val, const double beta, int64_t * y_deg_ptr, int64_t * y_idx, double * y_val, int64_t * loc_ws_in, double * val_ws_in)
{
  int64_t * loc_ws = loc_ws_in;
  double * val_ws = val_ws_in;

  OMP(omp parallel shared(loc_ws, val_ws)) {
    setup_workspace (nv, &loc_ws, &val_ws);
    setup_sparse_y (beta, *y_deg_ptr, y_idx, y_val, loc_ws, val_ws);
    struct batch b = BATCH_INIT;

    OMP(omp for nowait)
      for (int64_t xk = 0; xk < x_deg; ++xk) {
        const int64_t i = x_idx[xk];
        const double alphaxi = ALPHAXI_VAL (alpha, x_val[xk]);
        if (alphaxi != 0.0) {
          const int64_t degi = stinger_outdegree_get (S, i);
          const double alphaxi_deg = DEGSCALE (alphaxi, degi);
          dspmTspv_accum (S, i, alphaxi_deg, y_deg_ptr, y_idx, val_ws, loc_ws, &b);
        }
      }
    flush (&b, y_deg_ptr, y_idx, loc_ws);
    OMP(omp barrier)

    pack_vals (*y_deg_ptr, y_idx, val_ws, y_val);
  }

  OMP(omp barrier);
  OMP(omp master) {
       if (!val_ws_in) free (val_ws);
       if (!loc_ws_in) free (loc_ws);
  }
}

void stinger_unit_dspmTspv_degscaled (const int64_t nv, const double alpha, const struct stinger *S, const int64_t x_deg, const int64_t * x_idx, const double * x_val, const double beta, int64_t * y_deg_ptr, int64_t * y_idx, double * y_val, int64_t * loc_ws_in, double * val_ws_in, int64_t * total_vol)
{
  static int64_t vol;
  int64_t * restrict loc_ws = loc_ws_in;
  double * restrict val_ws = val_ws_in;

  /* double t0, t1, t2, t3, tugh, tugh2; */

  OMP(omp master) vol = 0; /* rely on barriers in the setup */

  setup_workspace (nv, &loc_ws, &val_ws);
  setup_sparse_y (beta, *y_deg_ptr, y_idx, y_val, loc_ws, val_ws);
  struct batch b = BATCH_INIT;

  OMP(omp for nowait schedule(guided) reduction(+: vol)) // reduction(+: tugh, tugh2)")
    for (int64_t xk = 0; xk < x_deg; ++xk) {
      const int64_t i = x_idx[xk];
      const double alphaxi = ALPHAXI_VAL (alpha, x_val[xk]);
      /* const double ti1 = omp_get_wtime(); */
      const int64_t degi = stinger_outdegree_get (S, i);
      /* tugh += omp_get_wtime()-ti1; */
      if (alphaxi != 0.0 && degi > 0) {
        const double alphaxi_deg = DEGSCALE (alphaxi, degi);
        vol += degi;
        /* const double ti2 = omp_get_wtime(); */
        dspmTspv_unit_accum (S, i, alphaxi_deg, y_deg_ptr, y_idx, val_ws, loc_ws, &b);
        /* tugh2 += omp_get_wtime()-ti2; */
      }
    }
  flush (&b, y_deg_ptr, y_idx, loc_ws);
  OMP(omp barrier);

  pack_vals (*y_deg_ptr, y_idx, val_ws, y_val);

  OMP(omp master) {
    *total_vol += vol;
    if (!val_ws_in) free (val_ws);
    if (!loc_ws_in) free (loc_ws);
  }

    /* OMP(omp single) fprintf (stderr, "x_deg %ld t1 %g t2 %g t3 %g  tugh %g tugh2 %g\n", (long)x_deg, t1-t0, t2-t1, t3-t2, tugh, tugh2); */
}


void stinger_unit_dspmTspv_degscaled_held (const double holdthresh, const int64_t nv, const double alpha, const struct stinger *S, const int64_t x_deg, const int64_t * x_idx, const double * x_val, const double beta, int64_t * y_deg_ptr, int64_t * y_idx, double * y_val, int64_t * loc_ws_in, double * val_ws_in, int64_t * total_vol)
{
  static int64_t vol;
  int64_t * restrict loc_ws = loc_ws_in;
  double * restrict val_ws = val_ws_in;

  /* double t0, t1, t2, t3, tugh, tugh2; */

  OMP(omp master) vol = 0; /* rely on barriers in the setup */

  setup_workspace (nv, &loc_ws, &val_ws);
  setup_sparse_y (beta, *y_deg_ptr, y_idx, y_val, loc_ws, val_ws);
  struct batch b = BATCH_INIT;

  OMP(omp for nowait schedule(guided) reduction(+: vol)) // reduction(+: tugh, tugh2)")
    for (int64_t xk = 0; xk < x_deg; ++xk) {
      const int64_t i = x_idx[xk];
      const double xi = x_val[xk];
      const double alphaxi = ALPHAXI_VAL (alpha, xi);
      /* const double ti1 = omp_get_wtime(); */
      const int64_t degi = stinger_outdegree_get (S, i);
      /* tugh += omp_get_wtime()-ti1; */
      if (fabs(xi) > holdthresh && degi > 0) {
        const double alphaxi_deg = DEGSCALE (alphaxi, degi);
        vol += degi;
        /* const double ti2 = omp_get_wtime(); */
        dspmTspv_unit_accum (S, i, alphaxi_deg, y_deg_ptr, y_idx, val_ws, loc_ws, &b);
        /* tugh2 += omp_get_wtime()-ti2; */
      } else {
        /* already in pattern... */
        //atomic_daccum (&val_ws[i], alphaxi);
      }
    }
  flush (&b, y_deg_ptr, y_idx, loc_ws);
  OMP(omp barrier);

  pack_vals (*y_deg_ptr, y_idx, val_ws, y_val);

  OMP(omp master) {
    *total_vol += vol;
    if (!val_ws_in) free (val_ws);
    if (!loc_ws_in) free (loc_ws);
  }

    /* OMP(omp single) fprintf (stderr, "x_deg %ld t1 %g t2 %g t3 %g  tugh %g tugh2 %g\n", (long)x_deg, t1-t0, t2-t1, t3-t2, tugh, tugh2); */
}

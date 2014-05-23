#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <assert.h>

#include "stinger_core/stinger_atomics.h"
#include "stinger_core/stinger.h"

#include "compat.h"

const static double NAN_EMPTY = nan ("0xDEADFACE");
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define MM_PAUSE() _mm_pause()
#else
#define MM_PAUSE() do { } while (0)
#endif

#if !defined(ATOMIC_FP_FE_EMUL) && !defined(ATOMIC_FP_OPTIMISTIC) && !defined(ATOMIC_FP_OMP)
#define ATOMIC_FP_OMP
/* #define ATOMIC_FP_FE_EMUL */
/* #define ATOMIC_FP_OPTIMISTIC */
#endif

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
  OMP("omp atomic") *p += val;
#endif
}

static inline void setup_y (const int64_t nv, const double beta, double * y)
{
  if (0.0 == beta) {
    OMP("omp for")
      for (int64_t i = 0; i < nv; ++i)
        y[i] = 0.0;
  } else if (-1.0 == beta) {
    OMP("omp for")
      for (int64_t i = 0; i < nv; ++i)
        y[i] = -y[i];
  } else if (!(1.0 == beta)) {
    OMP("omp for")
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
  STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
    const int64_t j = STINGER_EDGE_DEST;
    const double aij = STINGER_EDGE_WEIGHT;
    atomic_daccum (&y[j], aij * alphaxi);
  } STINGER_FORALL_EDGES_OF_VTX_END();
}

static inline void
dspmTv_unit_accum (const struct stinger * S, const int64_t i, const double alphaxi, double * y)
{
  STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
    const int64_t j = STINGER_EDGE_DEST;
    atomic_daccum (&y[j], alphaxi);
  } STINGER_FORALL_EDGES_OF_VTX_END();
}

void stinger_dspmTv_ompcas (const int64_t nv, const double alpha, const struct stinger *S, const double * x, const double beta, double * y)
{
  OMP("omp parallel if(!omp_in_parallel())") {
    setup_y (nv, beta, y);

    OMP("omp for")
      for (int64_t i = 0; i < nv; ++i) {
        const double alphaxi = ALPHAXI_VAL (alpha, x[i]);
        if (alphaxi != 0.0)
          dspmTv_accum (S, i, alphaxi, y);
      }
  }
}

void stinger_unit_dspmTv_ompcas (const int64_t nv, const double alpha, const struct stinger *S, const double * x, const double beta, double * y)
{
  OMP("omp parallel if(!omp_in_parallel())") {
    setup_y (nv, beta, y);

    OMP("omp for")
      for (int64_t i = 0; i < nv; ++i) {
        const double alphaxi = ALPHAXI_VAL (alpha, x[i]);
        if (alphaxi != 0.0)
          dspmTv_unit_accum (S, i, alphaxi, y);
      }
  }
}

void stinger_dspmTv_degscaled_ompcas (const int64_t nv, const double alpha, const struct stinger *S, const double * x, const double beta, double * y)
{
  OMP("omp parallel if(!omp_in_parallel())") {
    setup_y (nv, beta, y);

    OMP("omp for")
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

void stinger_unit_dspmTv_degscaled_ompcas (const int64_t nv, const double alpha, const struct stinger *S, const double * x, const double beta, double * y)
{
  OMP("omp parallel if(!omp_in_parallel())") {
    setup_y (nv, beta, y);

    OMP("omp for")
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

static void setup_workspace (const int64_t nv, int64_t ** loc_ws, double ** val_ws)
{
  if (!*loc_ws) {
    OMP("omp master") {
      *loc_ws = xmalloc (nv * sizeof (**loc_ws));
    }
    OMP("omp barrier");
    OMP("omp for")
      for (int64_t k = 0; k < nv; ++k) (*loc_ws)[k] = -1;
  }
  if (!*val_ws) {
    OMP("omp master") {
      *val_ws = xmalloc (nv * sizeof (**val_ws));
    }
    OMP("omp barrier");
    OMP("omp for")
      for (int64_t k = 0; k < nv; ++k) (*val_ws)[k] = 0.0;
  }
}

static void setup_sparse_y (const double beta,
                            int64_t y_deg, int64_t * y_idx, double * y_val,
                            int64_t * loc_ws, double * val_ws /*UNUSED*/)
{
  if (0.0 == beta) {
    OMP("omp for")
      for (int64_t k = 0; k < y_deg; ++k) {
        const int64_t i = y_idx[k];
        loc_ws[i] = k;
        y_val[k] = 0.0;
      }
  } else if (1.0 == beta) {
    /* Still have to set up the pattern... */
    OMP("omp for")
      for (int64_t k = 0; k < y_deg; ++k) {
        const int64_t i = y_idx[k];
        loc_ws[i] = k;
      }
  } else if (-1.0 == beta) {
    OMP("omp for")
      for (int64_t k = 0; k < y_deg; ++k) {
        const int64_t i = y_idx[k];
        loc_ws[i] = k;
        y_val[k] = -y_val[k];
      }
  } else if (1.0 != beta) {
    OMP("omp for")
      for (int64_t k = 0; k < y_deg; ++k) {
        const int64_t i = y_idx[k];
        const double yi = y_val[k];
        loc_ws[i] = k;
        y_val[k] = beta * y_val[k];
      }
  }
}

static inline void
dspmTspv_y_idx_accum (const struct stinger * S,
                      int64_t * y_deg, int64_t * y_idx,
                      const int64_t j,
                      int64_t * loc_ws)
{
#if !defined(NDEBUG)
  const int64_t nv = stinger_max_active_vertex (S) + 1;
#endif
  if (loc_ws[j] < 0) {
    int64_t where;
    int64_t expected = -1;
    int64_t desired = -2;
    if (__atomic_compare_exchange_n (&loc_ws[j], &expected, &desired, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)) {
    /* if (bool_int64_compare_and_swap (&loc_ws[j], -1, -2)) { */
      /* Own it. */
      /* where = int64_fetch_add (y_deg, 1); */
      where = __atomic_fetch_add (y_deg, 1, __ATOMIC_RELAXED);
      assert (where < nv);
      __atomic_store (&loc_ws[j], &where, __ATOMIC_RELEASE);
      y_idx[where] = j;
    }
  }
}

static inline void
dspmTspv_accum (const struct stinger * S, const int64_t i, const double alphaxi,
                int64_t * y_deg, int64_t * y_idx, double * y,
                int64_t * loc_ws)
{
  STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
    const int64_t j = STINGER_EDGE_DEST;
    const double aij = STINGER_EDGE_WEIGHT;
    atomic_daccum (&y[j], aij * alphaxi);
    dspmTspv_y_idx_accum (S, y_deg, y_idx, j, loc_ws);
  } STINGER_FORALL_EDGES_OF_VTX_END();
}

static inline void
dspmTspv_unit_accum (const struct stinger * S, const int64_t i, const double alphaxi,
                     int64_t * y_deg, int64_t * y_idx, double * y,
                     int64_t * loc_ws)
{
  STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
    const int64_t j = STINGER_EDGE_DEST;
    atomic_daccum (&y[j], alphaxi);
    dspmTspv_y_idx_accum (S, y_deg, y_idx, j, loc_ws);
  } STINGER_FORALL_EDGES_OF_VTX_END();
}

static inline void
pack_vals (const int64_t y_deg, const int64_t * restrict y_idx,
           double * restrict val_ws, double * restrict y_val)
{
  /* Pack the values back into the shorter form. */
  OMP("omp for")
    for (int64_t k = 0; k < y_deg; ++k) {
      y_val[k] = val_ws[y_idx[k]];
      val_ws[y_idx[k]] = 0.0;
    }
}

void stinger_dspmTspv_ompcas (const int64_t nv, const double alpha, const struct stinger *S, const int64_t x_deg, const int64_t * x_idx, const double * x_val, const double beta, int64_t * y_deg_ptr, int64_t * y_idx, double * y_val, int64_t * loc_ws_in, double * val_ws_in /*UNUSED*/)
{
  int64_t y_deg = * y_deg_ptr;
  int64_t * loc_ws = loc_ws_in;
  double * restrict val_ws = val_ws_in;

  OMP("omp parallel if(!omp_in_parallel())") {
    setup_workspace (nv, &loc_ws, &val_ws);
    setup_sparse_y (beta, y_deg, y_idx, y_val, loc_ws, val_ws);

    OMP("omp for")
      for (int64_t xk = 0; xk < x_deg; ++xk) {
        const int64_t i = x_idx[xk];
        const double alphaxi = ALPHAXI_VAL (alpha, x_val[xk]);
        if (alphaxi != 0.0)
          dspmTspv_accum (S, i, alphaxi, &y_deg, y_idx, val_ws, loc_ws);
      }

    pack_vals (y_deg, y_idx, val_ws, y_val);
  }

  if (!val_ws_in) free (val_ws);
  if (!loc_ws_in) free (loc_ws);
  *y_deg_ptr = y_deg;
}

void stinger_unit_dspmTspv_ompcas (const int64_t nv, const double alpha, const struct stinger *S, const int64_t x_deg, const int64_t * x_idx, const double * x_val, const double beta, int64_t * y_deg_ptr, int64_t * y_idx, double * y_val, int64_t * loc_ws_in, double * val_ws_in /*UNUSED*/)
{
  int64_t y_deg = * y_deg_ptr;
  int64_t * restrict loc_ws = loc_ws_in;
  double * restrict val_ws = val_ws_in;

  OMP("omp parallel if(!omp_in_parallel())") {
    setup_workspace (nv, &loc_ws, &val_ws);
    setup_sparse_y (beta, y_deg, y_idx, y_val, loc_ws, val_ws);

    OMP("omp for")
      for (int64_t xk = 0; xk < x_deg; ++xk) {
        const int64_t i = x_idx[xk];
        const double alphaxi = ALPHAXI_VAL (alpha, x_val[xk]);
        if (alphaxi != 0.0)
          dspmTspv_unit_accum (S, i, alphaxi, &y_deg, y_idx, val_ws, loc_ws);
      }

    pack_vals (y_deg, y_idx, val_ws, y_val);
  }

  if (!val_ws_in) free (val_ws);
  if (!loc_ws_in) free (loc_ws);
  *y_deg_ptr = y_deg;
}

void stinger_dspmTspv_degscaled_ompcas (const int64_t nv, const double alpha, const struct stinger *S, const int64_t x_deg, const int64_t * x_idx, const double * x_val, const double beta, int64_t * y_deg_ptr, int64_t * y_idx, double * y_val, int64_t * loc_ws_in, double * val_ws_in /*UNUSED*/)
{
  int64_t y_deg = * y_deg_ptr;
  int64_t * loc_ws = loc_ws_in;
  double * restrict val_ws = val_ws_in;

  OMP("omp parallel if(!omp_in_parallel())") {
    setup_workspace (nv, &loc_ws, &val_ws);
    setup_sparse_y (beta, y_deg, y_idx, y_val, loc_ws, val_ws);

    OMP("omp for")
      for (int64_t xk = 0; xk < x_deg; ++xk) {
        const int64_t i = x_idx[xk];
        const double alphaxi = ALPHAXI_VAL (alpha, x_val[xk]);
        if (alphaxi != 0.0) {
          const int64_t degi = stinger_outdegree_get (S, i);
          const double alphaxi_deg = DEGSCALE (alphaxi, degi);
          dspmTspv_accum (S, i, alphaxi_deg, &y_deg, y_idx, val_ws, loc_ws);
        }
      }

    pack_vals (y_deg, y_idx, val_ws, y_val);
  }

  if (!val_ws_in) free (val_ws);
  if (!loc_ws_in) free (loc_ws);
  *y_deg_ptr = y_deg;
}

void stinger_unit_dspmTspv_degscaled_ompcas (const int64_t nv, const double alpha, const struct stinger *S, const int64_t x_deg, const int64_t * x_idx, const double * x_val, const double beta, int64_t * y_deg_ptr, int64_t * y_idx, double * y_val, int64_t * loc_ws_in, double * val_ws_in /*UNUSED*/)
{
  int64_t y_deg = * y_deg_ptr;
  int64_t * restrict loc_ws = loc_ws_in;
  double * restrict val_ws = val_ws_in;

  /* double t0, t1, t2, t3, tugh, tugh2; */

  OMP("omp parallel if(!omp_in_parallel())") { // shared(t0, t1, t2, t3, tugh, tugh2)") {
    /* OMP("omp single") t0 = omp_get_wtime (); */
    setup_workspace (nv, &loc_ws, &val_ws);
    setup_sparse_y (beta, y_deg, y_idx, y_val, loc_ws, val_ws);

    /* OMP("omp single") t1 = omp_get_wtime (); */
    OMP("omp for") // reduction(+: tugh, tugh2)")
      for (int64_t xk = 0; xk < x_deg; ++xk) {
        const int64_t i = x_idx[xk];
        const double alphaxi = ALPHAXI_VAL (alpha, x_val[xk]);
        /* const double ti1 = omp_get_wtime(); */
        const int64_t degi = stinger_outdegree_get (S, i);
        /* tugh += omp_get_wtime()-ti1; */
        if (alphaxi != 0.0 && degi > 0) {
          const double alphaxi_deg = DEGSCALE (alphaxi, degi);
          /* const double ti2 = omp_get_wtime(); */
          dspmTspv_unit_accum (S, i, alphaxi_deg, &y_deg, y_idx, val_ws, loc_ws);
          /* tugh2 += omp_get_wtime()-ti2; */
        }
      }

    /* OMP("omp single") t2 = omp_get_wtime (); */
    pack_vals (y_deg, y_idx, val_ws, y_val);

    /* OMP("omp single") t3 = omp_get_wtime (); */
    OMP("omp barrier");
    OMP("omp master") {
      if (!val_ws_in) free (val_ws);
      if (!loc_ws_in) free (loc_ws);
      *y_deg_ptr = y_deg;
    }

    /* OMP("omp single") fprintf (stderr, "x_deg %ld t1 %g t2 %g t3 %g  tugh %g tugh2 %g\n", (long)x_deg, t1-t0, t2-t1, t3-t2, tugh, tugh2); */
  }
}

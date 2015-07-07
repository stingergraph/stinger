#include <stdlib.h>
#include <stdint.h>
#include <float.h>
#include <math.h>

#include <assert.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"

#include "compat.h"
#include "spmspv.h"
#include "spmspv_ompsimple.h"
#include "spmspv_ompcas.h"
#include "spmspv_ompcas_batch.h"

static inline double
termthresh_pr (const int64_t nv, struct stinger *S)
{
  const double x = sqrt(DBL_EPSILON) / 8.0;
  return nv * x;
}

static inline double
termthresh_dpr (const int64_t nv, struct stinger *S)
{
  const double x = sqrt(DBL_EPSILON) / 8.0;
  return nv * x;
}

static inline double
safediv (const double numer, const double denom)
{
  if (denom != 0.0) return numer / denom;
  if (numer == 0.0) return 0.0;
  return HUGE_VAL; /* Avoid a raising a flag? */
}

static inline double
norm1 (const int64_t N, const double * restrict v)
{
  static double out;
  OMP("omp single") out = 0.0;
  OMP("omp for reduction(+: out)")
    for (int64_t k = 0; k < N; ++k)
      out += fabs (v[k]);
  return out;
}

static inline double
norm1_diff (const int64_t N, const double * restrict v, const double * restrict w)
{
  static double out;
  OMP("omp single") out = 0.0;
  OMP("omp for reduction(+: out)")
    for (int64_t k = 0; k < N; ++k)
      out += fabs (v[k] - w[k]);
  return out;
}

static inline void
vdiv (const double alpha, const int64_t nv, double * v)
{
  OMP("omp for")
    for (int64_t k = 0; k < nv; ++k)
      v[k] /= alpha;
}

static inline void
vcopy (const int64_t nv, const double * restrict v, double * restrict vout)
{
  OMP("omp parallel for" OMP_SIMD)
    for (int64_t k = 0; k < nv; ++k)
      vout[k] = v[k];
}

static inline void
vcopy_scale (const double alpha, const int64_t nv, const double * restrict v, double * restrict vout)
{
  OMP("omp parallel for" OMP_SIMD)
    for (int64_t k = 0; k < nv; ++k)
      vout[k] = alpha * v[k];
}

static int
pagerank_core (const int64_t nv,
               struct stinger * S, double * x_in, const double * restrict kv,
               const double alpha, const int maxiter,
               double * workspace)
{
  const double termthresh = termthresh_pr (nv, S);

  double * restrict xnew = workspace;
  double * restrict x = x_in;
  double norm1_x;
  int niter;
  double norm1_xnew_diff;
  double rho = 1.0;

  OMP("omp parallel") {
    OMP("omp single") norm1_x = 0.0;
    OMP("omp for reduction(+: norm1_x)")
      for (int kk = 0; kk < nv; ++kk)
        norm1_x += fabs(x[kk]);

    for (int k = 0; rho >= termthresh && k < maxiter; ++k) {
      OMP("omp master") niter = k+1;
      OMP("omp for nowait" OMP_SIMD)
        for (int64_t kk = 0; kk < nv; ++kk)
          xnew[kk] = kv[kk];
      OMP("omp single")
        norm1_xnew_diff = 0.0;
      /* implied barrier */

      stinger_unit_dspmTv_degscaled_ompcas_batch (nv, alpha, S, x, 1.0, xnew);

      OMP("omp for reduction(+: norm1_xnew_diff)" OMP_SIMD)
        for (int64_t kk = 0; kk < nv; ++kk)
          norm1_xnew_diff += fabs(xnew[kk] - x[kk]);

      OMP("omp master") rho = safediv (norm1_xnew_diff, norm1_x);

      /* Always use xnew as the new x, even if converged. */
      OMP("omp single") {
        double * t;
        t = xnew;
        xnew = x;
        x = t;
        norm1_x = 0.0;
      }
      OMP("omp for reduction(+: norm1_x)" OMP_SIMD)
        for (int64_t kk = 0; kk < nv; ++kk)
          norm1_x += fabs(x[kk]);
    }

    OMP("omp for" OMP_SIMD)
      for (int64_t kk = 0; kk < nv; ++kk)
        x[kk] /= norm1_x;

    if (x != x_in) { /* Copy back. */
      OMP("omp for" OMP_SIMD)
        for (int64_t k = 0; k < nv; ++k)
          x_in[k] = x[k];
    }
  }

  return niter;
}

int
pagerank (const int64_t nv,
          struct stinger * S, double * x_in, const double * restrict v,
          const double alpha, const int maxiter,
          double * workspace)
{
  /* const int64_t nv = stinger_max_active_vertex (S) + 1; */
  double * kv = &workspace[nv];
  vcopy_scale ((1.0 - alpha) / norm1 (nv, v), nv, v, kv);
  vcopy (nv, kv, x_in);
  return pagerank_core (nv, S, x_in, kv, alpha, maxiter, workspace);
}

int
pagerank_restart (const int64_t nv,
                  struct stinger * S, double * x_in, const double * restrict v,
                  const double alpha, const int maxiter,
                  double * workspace)
{
  /* const int64_t nv = stinger_max_active_vertex (S) + 1; */
  double * kv = &workspace[nv];
  vcopy_scale ((1.0 - alpha) / norm1 (nv, v), nv, v, kv);
  return pagerank_core (nv, S, x_in, kv, alpha, maxiter, workspace);
}

int
pagerank_dpr (const int64_t nv, struct stinger * S,
              int64_t * x_deg, int64_t * x_idx, double * x_val,
              const double alpha, const int maxiter,
              int64_t * b_deg, int64_t * b_idx, double * b_val,
              const double norm1_pr,
              int64_t * dpr_deg_in, int64_t * dpr_idx, double * dpr_val,
              int64_t * mark,
              int64_t * iworkspace,
              double * dworkspace,
              double * dzero_workspace,
              int64_t * total_vol_out)
{
  /* const int64_t nv = stinger_max_active_vertex (S) + 1; */
  double cb = 0.0;
  int64_t dpr_deg;

  /* double time_1, time_2; */

  int niter_out;
  int64_t total_vol = 0;

  int64_t new_dpr_deg = 0;
  int64_t *new_dpr_idx;
  double *new_dpr_val;
  double rho = HUGE_VAL, new_rho;

  new_dpr_idx = iworkspace;
  new_dpr_val = dworkspace;

  OMP("omp parallel") {
    /* Update b = b1 - b0 */
    stinger_unit_dspmTspv_degscaled_ompcas_batch (nv, alpha, S, *x_deg, x_idx, x_val,
                                                  -alpha, b_deg, b_idx, b_val,
                                                  mark, dzero_workspace, &total_vol);

    const int64_t bdeg = *b_deg;
    OMP("omp master") new_dpr_deg = dpr_deg = bdeg; /* Relies on barriers below */

    OMP("omp for reduction(+:cb)" OMP_SIMD)
      for (int64_t k = 0; k < bdeg; ++k)
        cb += fabs (b_val[k]);
    OMP("omp for"  OMP_SIMD)
      for (int64_t k = 0; k < bdeg; ++k) {
        b_val[k] /= cb;
        new_dpr_idx[k] = dpr_idx[k] = b_idx[k];
        new_dpr_val[k] = dpr_val[k] = b_val[k];
      }

    const double termthresh = termthresh_dpr (nv, S) * norm1_pr / cb;
    int niter;

    /* On each iteration, new_dpr_deg enters with dpr_deg's pattern and b's values. */
    for (niter = 0; niter < maxiter && rho >= termthresh; ++niter) {

      OMP("omp master") new_rho = 0.0; /* uses barriers in spmspv */

      stinger_unit_dspmTspv_degscaled_ompcas_batch (nv, alpha, S, dpr_deg, dpr_idx, dpr_val,
                                                    1.0,
                                                    &new_dpr_deg, new_dpr_idx, new_dpr_val,
                                                    mark, dzero_workspace, &total_vol);
      OMP("omp for reduction(+: new_rho)" OMP_SIMD)
        for (int64_t k = 0; k < new_dpr_deg; ++k) {
          /* XXX: Again, assuming pattern is being super-setted and kept in order. */
          assert(k >= dpr_deg || new_dpr_idx[k] == dpr_idx[k]);
          new_rho += (k < dpr_deg? fabs (new_dpr_val[k] - dpr_val[k]) : fabs (new_dpr_val[k]));
          /* XXX: not needed if swapping buffers... */
          assert(k >= dpr_deg || new_dpr_idx[k] == dpr_idx[k]);
          if (k >= dpr_deg) dpr_idx[k] = new_dpr_idx[k];
          dpr_val[k] = new_dpr_val[k];
          new_dpr_val[k] = (k < bdeg? b_val[k] : 0.0);
        }

      OMP("omp single") {
        dpr_deg = new_dpr_deg;
        rho = new_rho;
      }
    }
    OMP("omp master") niter_out = niter;

    OMP("omp for" OMP_SIMD)
      for (int64_t k = 0; k < dpr_deg; ++k)
        dpr_val[k] *= cb;
  }

  *dpr_deg_in = dpr_deg;

  *total_vol_out = total_vol;
  return niter_out;
}

int
pagerank_dpr_held (const int64_t nv, struct stinger * S,
                   int64_t * x_deg, int64_t * x_idx, double * x_val,
                   const double alpha, const int maxiter,
                   int64_t * b_deg, int64_t * b_idx, double * b_val,
                   const double norm1_pr,
                   int64_t * dpr_deg_in, int64_t * dpr_idx, double * dpr_val,
                   int64_t * mark,
                   int64_t * iworkspace,
                   double * dworkspace,
                   double * dzero_workspace,
                   int64_t * total_vol_out)
{
  /* const int64_t nv = stinger_max_active_vertex (S) + 1; */
  double cb = 0.0;
  int64_t dpr_deg;

  /* double time_1, time_2; */

  int niter_out;
  int64_t total_vol = 0;

  int64_t new_dpr_deg = 0;
  int64_t *new_dpr_idx;
  double *new_dpr_val;
  double rho = HUGE_VAL, new_rho;

  new_dpr_idx = iworkspace;
  new_dpr_val = dworkspace;

  double ndpr = 0.0;

  OMP("omp parallel") {
    /* Update b = b1 - b0 */
    stinger_unit_dspmTspv_degscaled_ompcas_batch (nv, alpha, S, *x_deg, x_idx, x_val,
                                                  -alpha, b_deg, b_idx, b_val,
                                                  mark, dzero_workspace, &total_vol);

    const int64_t bdeg = *b_deg;
    OMP("omp master") new_dpr_deg = dpr_deg = bdeg; /* Relies on barriers below */

    OMP("omp for reduction(+:cb)" OMP_SIMD)
      for (int64_t k = 0; k < bdeg; ++k)
        cb += fabs (b_val[k]);
    OMP("omp for reduction(+:ndpr)" OMP_SIMD)
      for (int64_t k = 0; k < bdeg; ++k) {
        b_val[k] /= cb;
        new_dpr_idx[k] = dpr_idx[k] = b_idx[k];
        new_dpr_val[k] = dpr_val[k] = b_val[k];
        ndpr += b_val[k];
      }

    const double termthresh = termthresh_dpr (nv, S) * norm1_pr / cb;
    const double holdthreshmult = 8.0;
    int niter;

    /* On each iteration, new_dpr_deg enters with dpr_deg's pattern and b's values. */
    for (niter = 0; niter < maxiter && rho >= termthresh; ++niter) {

      OMP("omp master") new_rho = 0.0; /* uses barriers in spmspv */
      const double holdthresh = holdthreshmult * dpr_deg * DBL_EPSILON / ndpr;

      stinger_unit_dspmTspv_degscaled_held_ompcas_batch (holdthresh,
                                                         nv, alpha, S, dpr_deg, dpr_idx, dpr_val,
                                                         1.0,
                                                         &new_dpr_deg, new_dpr_idx, new_dpr_val,
                                                         mark, dzero_workspace, &total_vol);
      OMP("omp single") ndpr = 0.0;
      OMP("omp for reduction(+: new_rho, ndpr)" OMP_SIMD)
        for (int64_t k = 0; k < new_dpr_deg; ++k) {
          /* XXX: Again, assuming pattern is being super-setted and kept in order. */
          assert(k >= dpr_deg || new_dpr_idx[k] == dpr_idx[k]);
          new_rho += (k < dpr_deg? fabs (new_dpr_val[k] - dpr_val[k]) : fabs (new_dpr_val[k]));
          /* XXX: not needed if swapping buffers... */
          assert(k >= dpr_deg || new_dpr_idx[k] == dpr_idx[k]);
          if (k >= dpr_deg) dpr_idx[k] = new_dpr_idx[k];
          dpr_val[k] = new_dpr_val[k];
          new_dpr_val[k] = (k < bdeg? b_val[k] : 0.0);
          ndpr += dpr_val[k];
        }

      OMP("omp single") {
        dpr_deg = new_dpr_deg;
        rho = new_rho;
      }
    }
    OMP("omp master") niter_out = niter;

    OMP("omp for" OMP_SIMD)
      for (int64_t k = 0; k < dpr_deg; ++k)
        dpr_val[k] *= cb;
  }

  *dpr_deg_in = dpr_deg;

  *total_vol_out = total_vol;
  return niter_out;
}

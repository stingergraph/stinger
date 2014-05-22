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
  double out = 0.0;
  OMP("omp parallel for reduction(+: out)")
    for (int64_t k = 0; k < N; ++k)
      out += fabs (v[k]);
  return out;
}

static inline double
norm1_diff (const int64_t N, const double * restrict v, const double * restrict w)
{
  double out = 0.0;
  OMP("omp parallel for reduction(+: out)")
    for (int64_t k = 0; k < N; ++k)
      out += fabs (v[k] - w[k]);
  return out;
}

static inline void
vdiv (const double alpha, const int64_t nv, double * v)
{
  OMP("omp parallel if(!omp_in_parallel())")
    OMP("omp for")
    for (int64_t k = 0; k < nv; ++k)
      v[k] /= alpha;
}

static inline void
vcopy (const int64_t nv, const double * restrict v, double * restrict vout)
{
  OMP("omp parallel if(!omp_in_parallel())")
    OMP("omp for")
    for (int64_t k = 0; k < nv; ++k)
      vout[k] = v[k];
}

static inline void
vcopy_scale (const double alpha, const int64_t nv, const double * restrict v, double * restrict vout)
{
  OMP("omp parallel if(!omp_in_parallel())")
    OMP("omp for")
    for (int64_t k = 0; k < nv; ++k)
      vout[k] = alpha * v[k];
}

static int
pagerank_core (const int64_t nv,
               struct stinger * S, double * x_in, const double * restrict kv,
               const double alpha, const int maxiter,
               double * workspace)
{
  const double termthresh = sqrt (nv) * DBL_EPSILON;

  double * xnew = workspace;
  double * x = x_in;
  double norm1_x = norm1 (nv, x);
  double rho = 1.0;
  int niter;

  for (int k = 0; rho >= termthresh && k < maxiter; ++k) {
    double norm1_xnew_diff = 0;
    double * t;

    niter = k+1;
    vcopy (nv, kv, xnew);

    stinger_unit_dspmTv_degscaled_ompcas (nv, alpha, S, x, 1.0-alpha, xnew);

    norm1_xnew_diff = norm1_diff (nv, xnew, x);
    rho = safediv (norm1_xnew_diff, norm1_x);

    /* Always use xnew as the new x, even if converged. */
    t = xnew;
    xnew = x;
    x = t;
    norm1_x = norm1 (nv, x);
  }

  vdiv (norm1_x, nv, x);

  if (x != x_in) { /* Copy back. */
    OMP("omp parallel for")
      for (int64_t k = 0; k < nv; ++k)
        x_in[k] = x[k];
  }

  return niter;
}

int
pagerank (struct stinger * S, double * x_in, const double * restrict v,
          const double alpha, const int maxiter,
          double * workspace)
{
  /* const int64_t nv = stinger_max_active_vertex (S) + 1; */
  const int64_t nv = stinger_max_nv (S);
  double * kv = &workspace[nv];
  vcopy_scale (1.0 / norm1 (nv, v), nv, v, kv);
  vcopy (nv, kv, x_in);
  return pagerank_core (nv, S, x_in, kv, alpha, maxiter, workspace);
}

int
pagerank_restart (struct stinger * S, double * x_in, const double * restrict v,
                  const double alpha, const int maxiter,
                  double * workspace)
{
  /* const int64_t nv = stinger_max_active_vertex (S) + 1; */
  const int64_t nv = stinger_max_nv (S);
  double * kv = &workspace[nv];
  vcopy_scale ((1.0 - alpha) / norm1 (nv, v), nv, v, kv);
  return pagerank_core (nv, S, x_in, kv, alpha, maxiter, workspace);
}

int
pagerank_dpr (struct stinger * S,
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
  const int64_t nv = stinger_max_nv (S);
  const double termthresh = sqrt (nv) * DBL_EPSILON;
  double cb = 0.0;
  int64_t dpr_deg;

  /* double time_1, time_2; */

  int niter_out;
  int64_t total_vol = 0;

  int64_t new_dpr_deg = 0;
  int64_t *new_dpr_idx;
  double *new_dpr_val;
  double rho = HUGE_VAL;

  new_dpr_idx = iworkspace;
  new_dpr_val = dworkspace;

  int niter;
  /* OMP("omp master") time_1 = toc (); */
  /* Update b = b1 - b0 */
  stinger_unit_dspmTspv_degscaled_ompcas (nv, 1.0, S, *x_deg, x_idx, x_val,
                                          -1.0, b_deg, b_idx, b_val, mark, dzero_workspace);
  const int64_t bdeg = *b_deg;

    /* OMP("omp master") { time_2 = toc (); fprintf (stderr, "time 1 %g\n", time_2 - time_1); } */

  OMP("omp parallel") {
    OMP("omp for")
      for (int64_t k = 0; k < bdeg; ++k)
        cb += fabs (b_val[k]);
    OMP("omp for")
      for (int64_t k = 0; k < bdeg; ++k)
        b_val[k] /= cb;

    /* if (isnan(cb)) abort (); */

    OMP("omp for")
      for (int64_t k = 0; k < bdeg; ++k) {
        dpr_idx[k] = b_idx[k];
        dpr_val[k] = b_val[k];
      }
  }

  dpr_deg = bdeg;

  for (int niter = 0; niter < maxiter && rho >= termthresh; ++niter) {
    niter_out = niter+1;
    new_dpr_deg = dpr_deg;
    OMP("omp parallel") {
      OMP("omp for nowait reduction(+: total_vol)")
        for (int64_t k = 0; k < dpr_deg; ++k)
          total_vol += stinger_outdegree_get (S, dpr_idx[k]);

      /* fprintf (stderr, "%d: whoa, what?   %ld\n", niter, (long)total_vol); */

      /* XXX: Assume pattern is only appended... */
      OMP("omp for")
        for (int64_t k = 0; k < dpr_deg; ++k) {
          new_dpr_idx[k] = dpr_idx[k];
          new_dpr_val[k] = (k < bdeg? b_val[k] : 0.0);
        }
    }

    /* OMP("omp master") time_1 = toc (); */
    stinger_unit_dspmTspv_degscaled_ompcas (nv, alpha, S, dpr_deg, dpr_idx, dpr_val,
                                            1.0,
                                            &new_dpr_deg, new_dpr_idx, new_dpr_val,
                                            mark, dzero_workspace);
    /* OMP("omp master") { time_2 = toc (); fprintf (stderr, "%d: time 2 %g\n", niter, time_2 - time_1); } */

    /* fprintf (stderr, "%d: after dspmTspv\n", niter); */

    rho = 0;
    OMP("omp parallel") {
      OMP("omp for reduction(+: rho)")
        for (int64_t k = 0; k < dpr_deg; ++k)
          rho += fabs (new_dpr_val[k] - dpr_val[k]);
      OMP("omp for reduction(+: rho)")
        for (int64_t k = dpr_deg; k < new_dpr_deg; ++k)
          rho += fabs (new_dpr_val[k]);
    }

    /* nowait because the next loop uses new_dpr_deg... */
    rho *= cb / norm1_pr;
    /* fprintf (stderr, "rho %g    %g %g\n", rho, cb, norm1_pr); */
    dpr_deg = new_dpr_deg;

    OMP("omp parallel for")
      for (int64_t k = 0; k < new_dpr_deg; ++k) {
        dpr_idx[k] = new_dpr_idx[k];
        dpr_val[k] = new_dpr_val[k];
      }
  }

  OMP("omp parallel for")
    for (int64_t k = 0; k < dpr_deg; ++k)
      dpr_val[k] *= cb;

  *dpr_deg_in = dpr_deg;

  *total_vol_out = total_vol;
  return niter_out;
}

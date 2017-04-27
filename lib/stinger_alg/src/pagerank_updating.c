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
#include "stinger_alg/spmv_spmspv.h"

//#include "compat.h"

// HACK enable alternate OMP macro
#include "stinger_core/alternative_omp_macros.h"

//#define USE_INFNORM
#if defined(USE_INFNORM)
#define NORM_REDUCE max
#else
#define NORM_REDUCE +
#endif

static inline double
termthresh_pr (const int64_t nv, struct stinger *S)
{
  const double x =
#if defined(USE_INFNORM)
       4096.0*DBL_EPSILON;
#else
  //1.0e-5;
  128.0*FLT_EPSILON;
#endif
  return x;
}

static inline double
termthresh_dpr (const int64_t nv, struct stinger *S)
{
  const double x =
#if defined(USE_INFNORM)
       4096.0*DBL_EPSILON;
#else
  //1.0e-5;
  128.0*FLT_EPSILON;
#endif
  return x;
}

OMP(omp declare simd)
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
  OMP(omp single) out = 0.0;
  OMP(omp for OMP_SIMD reduction(+: out))
    for (int64_t k = 0; k < N; ++k)
      out += fabs (v[k]);
  return out;
}

static inline double
norm1_diff (const int64_t N, const double * restrict v, const double * restrict w)
{
  static double out;
  OMP(omp single) out = 0.0;
  OMP(omp for OMP_SIMD reduction(+: out))
    for (int64_t k = 0; k < N; ++k)
      out += fabs (v[k] - w[k]);
  return out;
}

static inline void
vdiv (const double alpha, const int64_t nv, double * v)
{
  OMP(omp for OMP_SIMD)
    for (int64_t k = 0; k < nv; ++k)
      v[k] /= alpha;
}

static inline void
vcopy (const int64_t nv, const double * restrict v, double * restrict vout)
{
  OMP(omp parallel for OMP_SIMD)
    for (int64_t k = 0; k < nv; ++k)
      vout[k] = v[k];
}

static inline void
vcopy_scale (const double alpha, const int64_t nv, const double * restrict v, double * restrict vout)
{
  OMP(omp parallel for OMP_SIMD)
    for (int64_t k = 0; k < nv; ++k)
      vout[k] = alpha * v[k];
}

static int
pagerank_core (const int64_t nv,
               struct stinger * S, double * x_in, const double * restrict kv,
               double * residual_in,
               const double alpha, const int maxiter)
{
  const double termthresh = termthresh_pr (nv, S);

  double * restrict xnew = residual_in;
  double * restrict x = x_in;
  double norm1_x;
  int niter;
  double norm1_xnew_diff;
  double rho = 1.0;

  OMP(omp parallel) {
    OMP(omp single) norm1_x = 0.0;
    OMP(omp for OMP_SIMD reduction(+: norm1_x))
      for (int kk = 0; kk < nv; ++kk)
        norm1_x += fabs(x[kk]);

    for (int k = 0; k < maxiter; ++k) {
      OMP(omp master) niter = k+1;
      OMP(omp for OMP_SIMD nowait)
        for (int64_t kk = 0; kk < nv; ++kk)
          xnew[kk] = kv[kk];
      OMP(omp single)
        norm1_xnew_diff = 0.0;
      /* implied barrier */

      stinger_unit_dspmTv_degscaled (nv, -alpha, S, x, 1.0, xnew);

#if defined(USE_INFNORM)
      OMP(omp for OMP_SIMD reduction(NORM_REDUCE : norm1_xnew_diff))
        for (int64_t kk = 0; kk < nv; ++kk) {
          double tmp = fabs(xnew[kk] - x[kk]); // /x[kk];
          if (tmp > norm1_xnew_diff) norm1_xnew_diff = tmp;
        }

      OMP(omp master) rho = norm1_xnew_diff;
#else
      OMP(omp for OMP_SIMD reduction(NORM_REDUCE : norm1_xnew_diff))
        for (int64_t kk = 0; kk < nv; ++kk)
          norm1_xnew_diff += fabs(xnew[kk] - x[kk]);

      OMP(omp master) rho = safediv (norm1_xnew_diff, norm1_x);
#endif

      if (rho < termthresh) break;

      OMP(omp single) {
        double * t;
        t = xnew;
        xnew = x;
        x = t;
        norm1_x = 0.0;
      }
      OMP(omp for OMP_SIMD reduction(+: norm1_x))
        for (int64_t kk = 0; kk < nv; ++kk)
          norm1_x += fabs(x[kk]);
    }

/*
    OMP(omp for OMP_SIMD)
      for (int64_t kk = 0; kk < nv; ++kk)
        x[kk] /= norm1_x;
*/

    if (x != x_in) { /* Copy back and compute residual. */
      /* x == residual_in, xnew == x_in */
      OMP(omp for OMP_SIMD)
        for (int64_t k = 0; k < nv; ++k) {
          double t;
          t = x_in[k];
          x_in[k] = x[k];
          x[k] = t - x[k];
        }
    } else { /* Just compute the residual. */
      /* x == x_in, xnew == residual_in */
      OMP(omp for OMP_SIMD)
        for (int64_t k = 0; k < nv; ++k)
          xnew[k] -= x[k];
    }
  }

  return niter;
}

int
pagerank (const int64_t nv,
          struct stinger * S, double * x_in, const double * restrict v,
          double * residual_in,
          const double alpha, const int maxiter,
          double * workspace)
{
  /* const int64_t nv = stinger_max_active_vertex (S) + 1; */
  double * kv = workspace;
  vcopy_scale ((1.0 - alpha) / norm1 (nv, v), nv, v, kv);
  //vcopy_scale (1.0 / norm1 (nv, v), nv, v, kv);
  vcopy (nv, kv, x_in);
  return pagerank_core (nv, S, x_in, kv, residual_in, alpha, maxiter);
}

int
pagerank_restart (const int64_t nv,
                  struct stinger * S, double * x_in, const double * restrict v,
                  double * residual_in,
                  const double alpha, const int maxiter,
                  double * workspace)
{
  /* const int64_t nv = stinger_max_active_vertex (S) + 1; */
  double * kv = workspace;
  int out;
  /* XXX: alpha*nu for dangling nodes */
  vcopy_scale ((1.0 - alpha) / norm1 (nv, v), nv, v, kv);
  //vcopy_scale (1.0 / norm1 (nv, v), nv, v, kv);
  out = pagerank_core (nv, S, x_in, kv, residual_in, alpha, maxiter);
  return out;
}

static void
update_residual (const int64_t nv, struct stinger * S,
                 const double alpha,
                 const int64_t bdeg, const int64_t * b_idx, const double * b_val, const double cb,
                 const int64_t dpr_deg, const int64_t * dpr_idx, const double * dpr_val,
                 double * residual,
                 int64_t * mark,
                 int64_t * iworkspace,
                 double * dworkspace,
                 double * dzero_workspace,
                 int64_t * total_vol_out)
{
  /* Update residual... Cheating method. */
  int64_t dres_deg = 0;
  int64_t * restrict dres_idx = iworkspace;
  double * restrict dres_val = dworkspace;

  OMP(omp parallel for)
    for (int64_t k = 0; k < dpr_deg; ++k) {
      dres_idx[k] = dpr_idx[k];
      dres_val[k] = -dpr_val[k];
      if (k < bdeg)
        dres_val[k] += b_val[k]*cb;
    }
  dres_deg = dpr_deg;

  stinger_unit_dspmTspv_degscaled (nv, alpha, S, dpr_deg, dpr_idx, dpr_val,
                                   1.0,
                                   &dres_deg, dres_idx, dres_val,
                                   mark, dzero_workspace, total_vol_out);

  OMP(omp parallel for)
    for (int64_t k = 0; k < dres_deg; ++k) {
      const int64_t i = dres_idx[k];
      residual[i] += dres_val[k];
      if (k >= dpr_deg) mark[i] = -1;
    }
}

int
pagerank_dpr (const int64_t nv, struct stinger * S,
              int64_t * x_deg, int64_t * x_idx, double * x_val,
              const double alpha, const int maxiter,
              int64_t * b_deg, int64_t * b_idx, double * b_val,
              int64_t * dpr_deg_in, int64_t * dpr_idx, double * dpr_val,
              double * residual_in,
              const double * old_pr_in,
              int64_t * mark,
              int64_t * iworkspace,
              double * dworkspace,
              double * dzero_workspace,
              int64_t * total_vol_out)
{
  /* const int64_t nv = stinger_max_active_vertex (S) + 1; */
  double cb = 0.0;
  int64_t dpr_deg;
  double last_frontier;

  const double * restrict old_pr = old_pr_in;
  double * restrict residual = residual_in;

  /* double time_1, time_2; */

  int niter_out;
  int64_t total_vol = 0;

  int64_t new_dpr_deg = 0;
  int64_t *new_dpr_idx;
  double *new_dpr_val;
  double rho = HUGE_VAL, new_rho;

  new_dpr_idx = iworkspace;
  new_dpr_val = dworkspace;

  OMP(omp parallel) {
    /* Update b = b1 - b0 */
    stinger_unit_dspmTspv_degscaled (nv, alpha, S, *x_deg, x_idx, x_val,
                                     -alpha, b_deg, b_idx, b_val,
                                     mark, dzero_workspace, &total_vol);

    const int64_t bdeg = *b_deg;
    OMP(omp master) new_dpr_deg = dpr_deg = bdeg; /* Relies on barriers below */

    OMP(omp for OMP_SIMD reduction(+:cb))
      for (int64_t k = 0; k < bdeg; ++k)
        cb += fabs (b_val[k]);
    OMP(omp single) cb = 1.0;
    OMP(omp for OMP_SIMD)
      for (int64_t k = 0; k < bdeg; ++k) {
        b_val[k] /= cb;
        new_dpr_idx[k] = dpr_idx[k] = b_idx[k];
        new_dpr_val[k] = dpr_val[k] = b_val[k];
      }

    const double termthresh = termthresh_dpr (nv, S) / cb / (1+alpha);
    int niter;

    /* On each iteration, new_dpr_deg enters with dpr_deg's pattern and b's values. */
    for (niter = 0; niter < maxiter && rho >= (termthresh*dpr_deg)/nv; ++niter) {

      OMP(omp master) { new_rho = 0.0; last_frontier = 0.0; } /* uses barriers in spmspv */

      stinger_unit_dspmTspv_degscaled (nv, alpha, S, dpr_deg, dpr_idx, dpr_val,
                                       1.0,
                                       &new_dpr_deg, new_dpr_idx, new_dpr_val,
                                       mark, dzero_workspace, &total_vol);
      OMP(omp for OMP_SIMD reduction(NORM_REDUCE : new_rho) reduction(+: last_frontier))
        for (int64_t k = 0; k < new_dpr_deg; ++k) {
          /* XXX: Again, assuming pattern is being super-setted and kept in order. */
          assert(k >= dpr_deg || new_dpr_idx[k] == dpr_idx[k]);
          new_dpr_val[k] += residual[new_dpr_idx[k]]/cb;
#if defined(USE_INFNORM)
          {
            const int64_t idx = new_dpr_idx[k];
            const double relval = 1.0; //old_pr[idx];
            const double tmp = (k < dpr_deg? fabs (new_dpr_val[k] - dpr_val[k]) : fabs (new_dpr_val[k]));
            double this_rho;
            if (relval) this_rho = tmp / relval;
            else if (tmp) this_rho = HUGE_VAL;
            else this_rho = 0.0;
            if (this_rho > new_rho) new_rho = this_rho;
          }
#else
          new_rho += (k < dpr_deg? fabs (new_dpr_val[k] - dpr_val[k]) : fabs (new_dpr_val[k]));
#endif
          /* XXX: not needed if swapping buffers... */
          assert(k >= dpr_deg || new_dpr_idx[k] == dpr_idx[k]);
          if (k >= dpr_deg) { dpr_idx[k] = new_dpr_idx[k]; last_frontier += cb*new_dpr_idx[k]; }
          dpr_val[k] = new_dpr_val[k];
          new_dpr_val[k] = (k < bdeg? b_val[k] : 0.0);
        }

      OMP(omp single) {
        dpr_deg = new_dpr_deg;
        rho = new_rho;
        /* fprintf (stderr, "RHO %g  dpr_deg %ld\n", rho, (long)dpr_deg); */
      }
    }
    OMP(omp master) niter_out = niter;

    OMP(omp for OMP_SIMD)
      for (int64_t k = 0; k < dpr_deg; ++k)
        dpr_val[k] *= cb;
  }


  double norm_rtick = 0.0, norm_dx = 0.0;
  OMP(omp parallel for OMP_SIMD reduction(+:norm_rtick) reduction(+:norm_dx))
    for (int64_t k = 0; k < dpr_deg; ++k) {
      if (k < *b_deg) {
        norm_rtick += fabs(cb*b_val[k] + residual[dpr_idx[k]]);
      } else {
        norm_rtick += fabs(residual[dpr_idx[k]]);
      }
      norm_dx += fabs(dpr_val[k]);
    }
  /* fprintf (stderr, "XXXXXX %g %g %g %g  ... %g\n",  */
  /*          norm_dx, norm_rtick, norm_dx/norm_rtick, norm_dx/norm_rtick*(1.0-alpha), */
  /*          norm_dx/norm_rtick*(1.0-alpha - last_frontier) ); */
  
  *dpr_deg_in = dpr_deg;
  *total_vol_out = total_vol;

  update_residual (nv, S, alpha, *b_deg, b_idx, b_val, cb,
                   dpr_deg, dpr_idx, dpr_val,
                   residual, mark, iworkspace, dworkspace, dzero_workspace, total_vol_out);

  return niter_out;
}

int
pagerank_dpr_held (const int64_t nv, struct stinger * S,
                   int64_t * x_deg, int64_t * x_idx, double * x_val,
                   const double alpha, const int maxiter,
                   int64_t * b_deg, int64_t * b_idx, double * b_val,
                   int64_t * dpr_deg_in, int64_t * dpr_idx, double * dpr_val,
                   double * residual_in,
                   const double * old_pr_in,
                   int64_t * mark,
                   int64_t * iworkspace,
                   double * dworkspace,
                   double * dzero_workspace,
                   int64_t * total_vol_out,
                   const double holdscale)
{
  /* const int64_t nv = stinger_max_active_vertex (S) + 1; */
  double cb = 0.0;
  int64_t dpr_deg;

  const double * restrict old_pr = old_pr_in;
  double * restrict residual = residual_in;

  /* double time_1, time_2; */

  int niter_out;
  int64_t total_vol = 0;

  int64_t new_dpr_deg = 0;
  int64_t *new_dpr_idx;
  double *new_dpr_val;
  double rho = HUGE_VAL, new_rho;

  new_dpr_idx = iworkspace;
  new_dpr_val = dworkspace;

  OMP(omp parallel) {
    /* Update b = b1 - b0 */
    stinger_unit_dspmTspv_degscaled (nv, alpha, S, *x_deg, x_idx, x_val,
                                     -alpha, b_deg, b_idx, b_val,
                                     mark, dzero_workspace, &total_vol);

    const int64_t bdeg = *b_deg;
    OMP(omp master) new_dpr_deg = dpr_deg = bdeg; /* Relies on barriers below */

    OMP(omp for OMP_SIMD reduction(+:cb))
      for (int64_t k = 0; k < bdeg; ++k)
        cb += fabs (b_val[k]);
    OMP(omp barrier); 
    OMP(omp single) { cb = 1.0; }
    OMP(omp barrier); 
    OMP(omp for OMP_SIMD)
      for (int64_t k = 0; k < bdeg; ++k) {
        const int64_t i = b_idx[k];
        b_val[k] /= cb;
        new_dpr_idx[k] = dpr_idx[k] = i;
        new_dpr_val[k] = dpr_val[k] = b_val[k] + residual[i]/cb;
      }

    const double termthresh = termthresh_dpr (nv, S) / cb; // /(1+alpha)
    const double holdthreshmult = holdscale * (1.0+alpha)/(1.0-alpha);
    int niter;

    /* On each iteration, new_dpr_deg enters with dpr_deg's pattern and b's values. */
    for (niter = 0; niter < maxiter && rho >= (termthresh * dpr_deg)/nv; ++niter) {

      OMP(omp master) new_rho = 0.0; /* uses barriers in spmspv */
      /* const double holdthresh = holdthreshmult * dpr_deg * DBL_EPSILON / ndpr; */
      //const double holdthresh = holdthreshmult * (1.0+alpha)/(1.0-alpha) * termthresh / dpr_deg;
      //const double holdthresh = (1.0-alpha)/(1.0+alpha) * termthresh / dpr_deg;
      const double holdthresh = holdthreshmult * termthresh / dpr_deg;

      //fprintf (stderr, "iter %d: %g %g %g %d %d %g\n", niter, cb, termthresh, holdthresh, (int)dpr_deg, (int)nv, dpr_deg/((double)nv));

      /* XXX: separate out the held vertices, then apply on the non-held vertices */
      /* somehow save the held vertices separately.  need dpr_deg? workspace... */
      /*   if nnz per row is sufficiently large (kinda small), separate filtering works for performance if mapped to the right processors */

      stinger_unit_dspmTspv_degscaled_held (holdthresh,
                                            nv, alpha, S, dpr_deg, dpr_idx, dpr_val,
                                            1.0,
                                            &new_dpr_deg, new_dpr_idx, new_dpr_val,
                                            mark, dzero_workspace, &total_vol);
      OMP(omp for OMP_SIMD reduction(NORM_REDUCE : new_rho))
        for (int64_t k = 0; k < new_dpr_deg; ++k) {
          /* XXX: Again, assuming pattern is being super-setted and kept in order. */
          assert(k >= dpr_deg || new_dpr_idx[k] == dpr_idx[k]);
          new_dpr_val[k] += residual[new_dpr_idx[k]]/cb;
#if defined(USE_INFNORM)
          {
            const int64_t idx = new_dpr_idx[k];
            const double relval = 1.0; //old_pr[idx];
            const double tmp = (k < dpr_deg? fabs (new_dpr_val[k] - dpr_val[k]) : fabs (new_dpr_val[k]));
            double this_rho;
            if (relval) this_rho = tmp / relval;
            else if (tmp) this_rho = HUGE_VAL;
            else this_rho = 0.0;
            if (this_rho > new_rho) new_rho = this_rho;
          }
#else
          new_rho += (k < dpr_deg? fabs (new_dpr_val[k] - dpr_val[k]) : fabs (new_dpr_val[k]));
#endif
          /* XXX: not needed if swapping buffers... */
          assert(k >= dpr_deg || new_dpr_idx[k] == dpr_idx[k]);
          if (k >= dpr_deg) dpr_idx[k] = new_dpr_idx[k];
          dpr_val[k] = new_dpr_val[k];
          new_dpr_val[k] = (k < bdeg? b_val[k] : 0.0);
        }

      OMP(omp single) {
        dpr_deg = new_dpr_deg;
        rho = new_rho;
      }
    }
    OMP(omp master) niter_out = niter;

    OMP(omp for OMP_SIMD)
      for (int64_t k = 0; k < dpr_deg; ++k)
        dpr_val[k] *= cb;
  }

  *dpr_deg_in = dpr_deg;
  *total_vol_out = total_vol;

  update_residual (nv, S, alpha, *b_deg, b_idx, b_val, cb,
                   dpr_deg, dpr_idx, dpr_val,
                   residual, mark, iworkspace, dworkspace, dzero_workspace, total_vol_out);

  return niter_out;
}


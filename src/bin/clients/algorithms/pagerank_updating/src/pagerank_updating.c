#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <assert.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"

#include "compat.h"
#include "pagerank.h"
#include "spmspv.h"
#include "spmspv_ompcas.h"

static inline int append_to_vlist (int64_t * restrict nvlist,
                                   int64_t * restrict vlist,
                                   int64_t * restrict mark /* |V|, init to -1 */,
                                   const int64_t i);
void clear_vlist (int64_t * restrict nvlist,
                  int64_t * restrict vlist,
                  int64_t * restrict mark);

static inline void normalize_pr (const int64_t nv, double * restrict pr_val);

static int nonunit_weights = 1;

struct spvect {
  int64_t nv;
  int64_t * idx;
  double * val;
};
static inline struct spvect alloc_spvect (int64_t nvmax);

#define BASELINE 0
#define RESTART 1
#define DPR 2
#define DPR_HELD 3
#define NPR_ALG 4

int
main(int argc, char *argv[])
{
  double init_time, gather_time, compute_b_time;
  double cwise_err;

  int iter = 0;
  int64_t nv;
  int64_t max_nv;

  double * pr_val[NPR_ALG];
  int niter[NPR_ALG];
  double pr_time[NPR_ALG];
  int64_t pr_vol[NPR_ALG];
  const char *pr_name[NPR_ALG] = {"pr", "pr_restart", "dpr", "dpr_held"};

  double alpha = 0.15;
  int maxiter = 100;

  struct spvect x;
  struct spvect b;
  struct spvect dpr;

  int64_t * mark;
  double * v;
  double * dzero_workspace;
  double * workspace;
  int64_t * iworkspace;

  for (int k = 1; k < argc; ++k) {
    if (0 == strcmp(argv[k], "--help") || 0 == strcmp(argv[1], "-h")) {
      fprintf (stderr,
               "spmspv_test [--unit]\n"
               "  --unit : Assume unit weight.\n");
    }
  }

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_registered_alg * alg =
    stinger_register_alg(
      .name="pagerank_updating",
      .data_per_vertex=4*sizeof(double),
      .data_description="dddd pr pr_restart dpr dpr_held",
      .host="localhost",
    );

  if(!alg) {
    LOG_E("Registering algorithm failed.  Exiting");
    return -1;
  }

  /* Assume the number of vertices does not change. */
  //nv = stinger_max_active_vertex (alg->stinger) + 1;
  max_nv = stinger_max_nv (alg->stinger);
  nv = max_nv;
  if (getenv ("NV")) {
    long nv_env = strtol (getenv ("NV"), NULL, 10);
    if (nv_env > 0 && nv_env < nv)
      nv = nv_env;
  }
  fprintf (stderr, "NV: %ld\n", (long)nv);

  for (int k = 0; k < NPR_ALG; ++k) {
    double * alg_data = (double*)alg->alg_data;
    pr_val[k] = &alg_data[k * max_nv];
    pr_vol[k] = 0;
    niter[k] = 0;
  }

  mark = xmalloc (nv * sizeof (*mark));
  v = xmalloc (nv * sizeof (*v));
  dzero_workspace = xcalloc (nv, sizeof (*dzero_workspace));
  workspace = xmalloc (2 * nv * sizeof (*workspace));
  iworkspace = xmalloc (nv * sizeof (*iworkspace));
  x = alloc_spvect (nv);
  b = alloc_spvect (nv);
  dpr = alloc_spvect (nv);

  OMP("omp parallel") {
    OMP("omp for nowait")
      for (int64_t i = 0; i < nv; ++i) {
        mark[i] = -1;
        for (int k = 0; k < 4; ++k)
          pr_val[k][i] = 0.0;
      }
    OMP("omp for nowait")
      for (int64_t i = 0; i < nv; ++i)
        v[i] = 1.0 / nv; /* XXX: Assuming nv doesn't change... */
  }

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {
    if (stinger_total_edges (alg->stinger)) {
      tic ();
      niter[BASELINE] = pagerank (nv, alg->stinger, pr_val[BASELINE], v, alpha, maxiter, workspace);
      pr_time[BASELINE] = toc ();
#if !defined(NDEBUG)
      for (int64_t k = 0; k < nv; ++k)
        assert (!isnan(pr_val[BASELINE][k]));
#endif
      normalize_pr (nv, pr_val[BASELINE]);
#if !defined(NDEBUG)
      for (int64_t k = 0; k < nv; ++k)
        assert (!isnan(pr_val[BASELINE][k]));
#endif
      pr_vol[BASELINE] = niter[BASELINE] * stinger_total_edges (alg->stinger);
      OMP("omp parallel") {
        for (int k = 1; k < NPR_ALG; ++k) {
          pr_time[k] = 0;
          pr_vol[k] = 0;
          OMP("omp for nowait")
            for (int64_t i = 0; i < nv; ++i)
              pr_val[k][i] = pr_val[BASELINE][i];
        }
      }
    }
  } stinger_alg_end_init(alg);


  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Streaming Phase
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  while(alg->enabled) {
    double b0n;
    ++iter;
    /* Pre processing */
    if(stinger_alg_begin_pre(alg)) {
      clear_vlist (&x.nv, x.idx, mark);
      OMP("omp parallel for")
        for (int64_t k = 0; k < nv; ++k) mark[k] = -1;
      /* Gather vertices that are affected by the batch. */
      /* (Really should be done in the framework, but ends up being handy here...) */
      tic ();
      {
        const int64_t nins = alg->num_insertions;
        const int64_t nrem = alg->num_deletions;
        const stinger_edge_update * restrict ins = alg->insertions;
        const stinger_edge_update * restrict rem = alg->deletions;

        OMP("omp parallel") {
          OMP("omp for nowait")
            for (int64_t k = 0; k < nins; ++k) {
              append_to_vlist (&x.nv, x.idx, mark, ins[k].source);
              append_to_vlist (&x.nv, x.idx, mark, ins[k].destination);
            }
          OMP("omp for")
            for (int64_t k = 0; k < nrem; ++k) {
              append_to_vlist (&x.nv, x.idx, mark, rem[k].source);
              append_to_vlist (&x.nv, x.idx, mark, rem[k].destination);
            }
          OMP("omp for")
            for (int64_t k = 0; k < x.nv; ++k) {
              assert(!isnan(pr_val[DPR][x.idx[k]]));
              if (isnan(pr_val[DPR][x.idx[k]])) abort ();
              x.val[k] = pr_val[DPR][x.idx[k]];
              mark[x.idx[k]] = -1;
            }
        }
      }
      gather_time = toc ();

      tic ();
      b.nv = 0;
      /* Compute b0 in b */
      stinger_unit_dspmTspv_degscaled_ompcas (nv,
                                              1.0, alg->stinger,
                                              x.nv, x.idx, x.val,
                                              0.0,
                                              &b.nv, b.idx, b.val,
                                              mark, dzero_workspace);
      OMP("omp parallel") {
        OMP("omp for reduction(+: b0n)")
          for (int64_t k = 0; k < b.nv; ++k) b0n += fabs (b.val[k]);
        OMP("omp for")
          for (int64_t k = 0; k < b.nv; ++k) mark[b.idx[k]] = -1;
      }
      compute_b_time = toc ();
      stinger_alg_end_pre(alg);
    }

    fprintf (stderr, "%ld: NV CHANGED %ld    %ld %ld   %g\n", (long)iter, (long)x.nv,
             (long)alg->num_insertions, (long)alg->num_deletions, b0n);

    /* Post processing */
    if(stinger_alg_begin_post(alg)) {
      struct stinger * S = alg->stinger;
      const int64_t NE = stinger_total_edges (S);

      //assert (nv == stinger_max_active_vertex (alg->stinger) + 1);

      /* Compute PageRank from scratch */
      tic ();
      niter[BASELINE] = pagerank (nv, S, pr_val[BASELINE], v, alpha, maxiter, workspace);
      normalize_pr (nv, pr_val[BASELINE]);
      pr_time[BASELINE] = toc ();
      pr_vol[BASELINE] = niter[BASELINE] * NE;

      tic ();
      niter[RESTART] = pagerank_restart (nv, S, pr_val[RESTART], v, alpha, maxiter, workspace);
      normalize_pr (nv, pr_val[RESTART]);
      pr_time[RESTART] = toc ();
      pr_vol[RESTART] = niter[RESTART] * NE;

      tic ();
      niter[DPR] = pagerank_dpr (nv, S,
                                 &x.nv, x.idx, x.val,
                                 alpha, maxiter,
                                 &b.nv, b.idx, b.val,
                                 1.0,
                                 &dpr.nv, dpr.idx, dpr.val,
                                 mark, iworkspace, workspace, dzero_workspace,
                                 &pr_vol[DPR]);
      OMP("omp parallel for")
        for (int64_t k = 0; k < dpr.nv; ++k)
          pr_val[DPR][dpr.idx[k]] += dpr.val[k];
      normalize_pr (nv, pr_val[DPR]);
      pr_time[DPR] = toc ();

      OMP("omp parallel for")
        for (int64_t k = 0; k < b.nv; ++k) mark[dpr.idx[k]] = -1;

      stinger_alg_end_post(alg);
    }

    fprintf (stderr, "%ld: b_time %g\n", (long)iter, compute_b_time);
    for (int alg = 0; alg <= DPR; ++alg) {
      double err = 0.0;
      if (alg > 0)
        OMP("omp parallel for")
          for (int64_t i = 0; i < nv; ++i)
            err += fabs (pr_val[alg][i] - pr_val[BASELINE][i]);
      fprintf (stderr, "%ld: %s %d %d %g %ld %g\n", (long)iter, pr_name[alg], alg, niter[alg], pr_time[alg], pr_vol[alg], err);
    }
  }

  LOG_I("Algorithm complete... shutting down");
}

/* Utility functions */

MTA("mta inline") MTA("mta expect parallel context")
int
append_to_vlist (int64_t * restrict nvlist,
                 int64_t * restrict vlist,
                 int64_t * restrict mark /* |V|, init to -1 */,
                 const int64_t i)
{
  int out = 0;

  /* for (int64_t k = 0; k < *nvlist; ++k) { */
  /*   assert (vlist[k] >= 0); */
  /*   if (k != mark[vlist[k]]) */
  /*     fprintf (stderr, "wtf mark[vlist[%ld] == %ld] == %ld\n", */
  /*              (long)k, (long)vlist[k], (long)mark[vlist[k]]); */
  /*   assert (mark[vlist[k]] == k); */
  /* } */

  if (mark[i] < 0) {
    int64_t where;
    if (bool_int64_compare_and_swap (&mark[i], -1, -2)) {
      assert (mark[i] == -2);
      /* Own it. */
      where = int64_fetch_add (nvlist, 1);
      mark[i] = where;
      vlist[where] = i;
      out = 1;
    }
  }

  return out;
}

void
clear_vlist (int64_t * restrict nvlist,
             int64_t * restrict vlist,
             int64_t * restrict mark)
{
  const int64_t nvl = *nvlist;

  OMP("omp parallel for")
    for (int64_t k = 0; k < nvl; ++k)
      mark[vlist[k]] = -1;

  *nvlist = 0;
}

struct spvect
alloc_spvect (int64_t nvmax)
{
  struct spvect out;
  out.nv = 0;
  out.idx = xmalloc (nvmax * sizeof (*out.idx));
  out.val = xmalloc (nvmax * sizeof (*out.val));
  return out;
}

void
normalize_pr (const int64_t nv, double * restrict pr_val)
{
  double n1 = 0;
  OMP("omp parallel") {
    OMP("omp for reduction(+: n1)")
      for (int64_t k = 0; k < nv; ++k)
        n1 += fabs (pr_val[k]);
    OMP("omp for")
      for (int64_t k = 0; k < nv; ++k)
        pr_val[k] /= n1;
  }
}

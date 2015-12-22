#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <assert.h>

#if defined(HAVE_VALGRIND_MEMCHECK_H)
#include <valgrind/memcheck.h>
#else
#define VALGRIND_MAKE_MEM_NOACCESS(p_, len_)
#define VALGRIND_MAKE_MEM_DEFINED(p_, len_)
#endif

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

// HACK enable alternate OMP macro
#include "stinger_core/alternative_omp_macros.h"

#define ALG_SEQ 0
#define ALG_OMPSIMPLE 1
#define ALG_OMPCAS 2
#define ALG_OMPCAS_BATCH 3
static int alg_to_try = ALG_SEQ;
static const char *alg_arg = "seq";

static inline int append_to_vlist (int64_t * restrict nvlist,
                                   int64_t * restrict vlist,
                                   int64_t * restrict mark /* |V|, init to -1 */,
                                   const int64_t i);
void clear_vlist (int64_t * restrict nvlist,
                  int64_t * restrict vlist,
                  int64_t * restrict mark);

static int nonunit_weights = 1;

/* Arbitrary choice of coefficients for now. */
static const double alpha = 0.3;
static const double beta = 0.7;

static int64_t iter = 0;

struct spvect {
  int64_t nv;
  int64_t * idx;
  double * val;
};
static inline struct spvect alloc_spvect (int64_t nvmax);

int
main(int argc, char *argv[])
{
  double init_time, mult_time, gather_time, spmult_time;
  double cwise_err, mult_cwise_err;

  int64_t nv;

  double * x_dense;

  double * y;
  double * y_copy;
  double * y_seq;

  struct spvect dy;
  struct spvect x;
  double * dense_x;

  int64_t * mark;
  double * val_ws;

  for (int k = 1; k < argc; ++k) {
    if (0 == strcmp(argv[k], "--unit"))
      nonunit_weights = 0;
    else if (0 == strcmp(argv[k], "--alg")) {
      ++k;
      if (0 == strcmp(argv[k], "seq")) {
        alg_to_try = ALG_SEQ;
        alg_arg = argv[k];
      } else if (0 == strcmp(argv[k], "ompsimple")) {
        alg_to_try = ALG_OMPSIMPLE;
        alg_arg = argv[k];
      } else if (0 == strcmp(argv[k], "ompcas")) {
        alg_to_try = ALG_OMPCAS;
        alg_arg = argv[k];
      } else if (0 == strcmp(argv[k], "ompcasbatch")) {
        alg_to_try = ALG_OMPCAS_BATCH;
        alg_arg = argv[k];
      } else {
        fprintf (stderr, "Unknown algorithm \"%s\".\n", argv[k]);
        abort ();
      }
    } else if (0 == strcmp(argv[k], "--help") || 0 == strcmp(argv[1], "-h")) {
      fprintf (stderr,
               "spmspv_test [--unit]\n"
               "  --unit : Assume unit weight.\n");
    }
  }

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  char namebuf[1024];
  snprintf(namebuf, 1024, "spmspv_test_%s", alg_arg);
  stinger_registered_alg * alg =
    stinger_register_alg(
      .name=namebuf,
      .data_per_vertex=2*sizeof(double),
      .data_description="dd spmspv-test-vect spmspv-test-x",
      .host="localhost",
    );

  if(!alg) {
    LOG_E("Registering algorithm failed.  Exiting");
    return -1;
  }

  /* Assume the number of vertices does not change. */
  nv = stinger_max_active_vertex(alg->stinger)+1;

  y = (double*)alg->alg_data;
  x_dense = &y[nv];
  memset (x_dense, 0, nv * sizeof (*x_dense));

  y_copy = xmalloc (nv * sizeof (*y_copy));
  y_seq = xmalloc (nv * sizeof (*y_seq));

  dy = alloc_spvect (nv);
  x = alloc_spvect (nv);
  dense_x = xmalloc (nv * sizeof (*dense_x));
  mark = xmalloc (nv * sizeof (*mark));
  val_ws = xcalloc (nv, sizeof (*val_ws));

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {
    tic ();
    OMP(parallel for)
      for (int64_t i = 0; i < nv; ++i) {
        y[i] = 0;
        mark[i] = -1;
      }
    init_time = toc ();
  } stinger_alg_end_init(alg);

  LOG_V_A("spmspv_test: alg = %d", alg_to_try);
  LOG_V_A("spmspv_test: init time = %g\n", init_time);

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Streaming Phase
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  while(alg->enabled) {
    int64_t vol_x = 0;
    int64_t vol_S = 0;
    ++iter;
    /* Pre processing */
    if(stinger_alg_begin_pre(alg)) {
      /* Gather vertices that are affected by the batch. */
      /* (Really should be done in the framework, but ends up being handy here...) */ 
      tic ();
      {
        const int64_t nins = alg->num_insertions;
        const int64_t nrem = alg->num_deletions;
        const stinger_edge_update * restrict ins = alg->insertions;
        const stinger_edge_update * restrict rem = alg->deletions;

        OMP(parallel) {
          OMP(for nowait)
            for (int64_t k = 0; k < nins; ++k) {
              append_to_vlist (&x.nv, x.idx, mark, ins[k].source);
              append_to_vlist (&x.nv, x.idx, mark, ins[k].destination);
            }
          OMP(for)
            for (int64_t k = 0; k < nrem; ++k) {
              append_to_vlist (&x.nv, x.idx, mark, rem[k].source);
              append_to_vlist (&x.nv, x.idx, mark, rem[k].destination);
            }
          OMP(for)
            for (int64_t k = 0; k < x.nv; ++k)
              x.val[k] = 1.0;
        }
      }
      gather_time = toc ();
      stinger_alg_end_pre(alg);
      OMP(parallel) {
        OMP(for)
          for (int64_t i = 0; i < nv; ++i)
            dense_x[i] = 0;
        OMP(for)
          for (int64_t k = 0; k < x.nv; ++k)
            dense_x[x.idx[k]] = 1.0;
      }
    }

    /* Post processing */
    if(stinger_alg_begin_post(alg)) {
      /* Set up the dense x, copy items, etc. */
      dy.nv = x.nv;
      OMP(parallel) {
        OMP(for)
          for (int64_t i = 0; i < nv; ++i) {
            /* Form the dense x for testing. */
            dense_x[i] = 0.0;
            y_copy[i] = y[i];
            y_seq[i] = y[i];
          }
        OMP(for)
          for (int64_t k = 0; k < x.nv; ++k) {
            dy.idx[k] = x.idx[k];
            dy.val[k] = 0.0;
            dense_x[x.idx[k]] = x.val[k];
          }
        /* Count volume *after* changing the graph. */
        OMP(for reduction(+:vol_x))
          for (int64_t k = 0; k < x.nv; ++k)
            vol_x += stinger_outdegree_get (alg->stinger, x.idx[k]);
      }
      vol_S = stinger_total_edges (alg->stinger);

#define SPMV_BRANCH(prodalg, PRODALG)                                   \
      case ALG_ ## PRODALG:                                             \
        if (nonunit_weights) {                                          \
          stinger_dspmTv_ ## prodalg (nv, alpha, alg->stinger, dense_x, beta, y); \
        } else {                                                        \
          stinger_unit_dspmTv_ ## prodalg (nv, alpha, alg->stinger, dense_x, beta, y); \
        }                                                               \
        break

#define SPMSPV_BRANCH(prodalg, PRODALG)                                 \
      case ALG_ ## PRODALG:                                             \
        if (nonunit_weights) {                                          \
          stinger_dspmTspv_ ## prodalg (nv, alpha, alg->stinger, x.nv, x.idx, x.val, 0.0, &dy.nv, dy.idx, dy.val, \
                            mark, val_ws);                              \
        } else {                                                        \
          stinger_unit_dspmTspv_ ## prodalg (nv, alpha, alg->stinger, x.nv, x.idx, x.val, 0.0, &dy.nv, dy.idx, dy.val, \
                                 mark, val_ws);                         \
        }                                                               \
        break

      if (nonunit_weights) {
        stinger_dspmTv (nv, alpha, alg->stinger, dense_x, beta, y_seq);
      } else {
        stinger_unit_dspmTv (nv, alpha, alg->stinger, dense_x, beta, y_seq);
      }

      VALGRIND_MAKE_MEM_NOACCESS(y_seq, nv*sizeof(*y_seq));

      tic ();
      switch (alg_to_try) {
      case ALG_SEQ:
        if (nonunit_weights) {
          stinger_dspmTv (nv, alpha, alg->stinger, dense_x, beta, y);
        } else {
          stinger_unit_dspmTv (nv, alpha, alg->stinger, dense_x, beta, y);
        }
        break;
        SPMV_BRANCH(ompsimple,OMPSIMPLE);
        SPMV_BRANCH(ompcas,OMPCAS);
        SPMV_BRANCH(ompcas_batch,OMPCAS_BATCH);
      }
      mult_time = toc ();

      VALGRIND_MAKE_MEM_NOACCESS(y, nv*sizeof(*y));

      tic ();
      switch (alg_to_try) {
      case ALG_SEQ:
        if (nonunit_weights) {
          stinger_dspmTspv (nv, alpha, alg->stinger, x.nv, x.idx, x.val, 0.0, &dy.nv, dy.idx, dy.val,
                            mark, val_ws);
        } else {
          stinger_unit_dspmTspv (nv, alpha, alg->stinger, x.nv, x.idx, x.val, 0.0, &dy.nv, dy.idx, dy.val,
                                 mark, val_ws);
        }
        break;
        SPMSPV_BRANCH(ompsimple,OMPSIMPLE);
        SPMSPV_BRANCH(ompcas,OMPCAS);
        SPMSPV_BRANCH(ompcas_batch,OMPCAS_BATCH);
      }
      spmult_time = toc ();

      VALGRIND_MAKE_MEM_DEFINED(y_seq, nv*sizeof(*y_seq));
      VALGRIND_MAKE_MEM_DEFINED(y, nv*sizeof(*y));

      /* Apply update to y_copy */
      OMP(parallel for)
        for (int64_t k = 0; k < dy.nv; ++k) {
          const int64_t i = dy.idx[k];
          assert (i >= 0);
          assert (i < nv);
          y_copy[i] = beta * y_copy[i] + dy.val[k];
        }
      spmult_time = toc ();

      mult_cwise_err = 0.0;
      OMP(parallel) {
        double t_cwise_err = 0.0;
        OMP(for nowait)
          for (int64_t i = 0; i < nv; ++i) {
            double err;

            if (y_seq[i] != 0.0)
              err = fabs ((y_seq[i] - y[i])/y_seq[i]);
            else if (y_copy[i] == 0.0)
              err = 0.0;
            else
              err = HUGE_VAL;

            if (err > t_cwise_err) t_cwise_err = err;
          }
        OMP(critical)
          if (t_cwise_err > mult_cwise_err) mult_cwise_err = t_cwise_err;
      }
      /* Compute componentwise relative error... which can only be in updated locations. */
      cwise_err = 0.0;
      OMP(parallel) {
        double t_cwise_err = 0.0;
        OMP(for nowait)
          for (int64_t k = 0; k < dy.nv; ++k) {
            int64_t i = dy.idx[k];
            double err;

            if (y_seq[i] != 0.0)
              err = fabs ((y_seq[i] - y_copy[i])/y_seq[i]);
            else if (y_copy[i] == 0.0)
              err = 0.0;
            else
              err = HUGE_VAL;

            if (err > t_cwise_err) t_cwise_err = err;
          }
        OMP(critical)
          if (t_cwise_err > cwise_err) cwise_err = t_cwise_err;
      }

      stinger_alg_end_post(alg);

      LOG_V_A("spmspv_test: gather_time %ld = %g", (long)iter, gather_time);
      LOG_V_A("spmspv_test: mult_time %ld = %g", (long)iter, mult_time);
      LOG_V_A("spmspv_test: mult_teps %ld = %g", (long)iter, vol_S / mult_time);
      LOG_V_A("spmspv_test: mult_cwise_err %ld = %g", (long)iter, mult_cwise_err);
      LOG_V_A("spmspv_test: spmult_time %ld = %g", (long)iter, spmult_time);
      LOG_V_A("spmspv_test: spmult_teps %ld = %g", (long)iter, vol_x / spmult_time);
      LOG_V_A("spmspv_test: spmult_cwise_err %ld = %g", (long)iter, cwise_err);

      clear_vlist (&dy.nv, dy.idx, mark);
      x.nv = 0;
    }
  }

  LOG_I("Algorithm complete... shutting down");
  xfree(alg);
}

/* Utility functions */


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

  OMP(parallel for)
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

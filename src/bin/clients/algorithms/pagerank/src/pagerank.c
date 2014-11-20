#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"

int64_t
page_rank (stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter)
{
  double * tmp_pr = NULL;
  if (tmp_pr_in) {
    tmp_pr = tmp_pr_in;
  } else {
    LOG_W("Did not pass a buffer, so we allocate one");
    tmp_pr = (double *)xmalloc(sizeof(double) * NV);
  }

  int64_t iter = maxiter;
  double delta = 1;
  int64_t iter_count = 0;
  double damping_vec = ( ( (double) (1-dampingfactor) ) / ( (double) NV ) );

  while (delta > epsilon && iter > 0) {
    iter_count++;
    delta = 0;

    OMP("omp parallel for")
    for (uint64_t v = 0; v < NV; v++) {
      double partial = 0;

      STINGER_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(S, v) {
        int64_t outdegree = stinger_outdegree (S, STINGER_EDGE_DEST);
        partial += ( ( (double) pr[STINGER_EDGE_DEST] ) / ( (double) (outdegree ? outdegree : NV-1) ) );
      } STINGER_PARALLEL_FORALL_EDGES_OF_VTX_END();

      partial = partial * dampingfactor + damping_vec;

      /* epsilon check */
      double mydelta = partial - pr[v];
      if (mydelta < 0) {
	mydelta = -mydelta;
      }
      delta += mydelta;

      /* write back result */
      pr[v] = partial;
    }

    iter--;
  }

  LOG_I_A("PageRank iteration count : %ld", iter_count);

  if (!tmp_pr_in)
    free(tmp_pr);
}

int64_t
page_rank_type(stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter, int64_t type)
{
  double * tmp_pr = NULL;
  if (tmp_pr_in) {
    tmp_pr = tmp_pr_in;
  } else {
    tmp_pr = (double *)xmalloc(sizeof(double) * NV);
  }

  int64_t iter = maxiter;
  double delta = 1;

  while (delta > epsilon && iter > 0) {
    OMP("omp parallel for")
    for (uint64_t v = 0; v < NV; v++) {
      tmp_pr[v] = 0;

      STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(S, type, v) {
	/* TODO: this should be typed outdegree */
        int64_t outdegree = stinger_outdegree (S, STINGER_EDGE_DEST);
	tmp_pr[v] += (((double) pr[STINGER_EDGE_DEST]) / 
	  ((double) (outdegree ? outdegree : NV-1)));
      } STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_END();
    }

    OMP("omp parallel for")
    for (uint64_t v = 0; v < NV; v++) {
      tmp_pr[v] = tmp_pr[v] * dampingfactor + (((double)(1-dampingfactor)) / ((double)NV));
    }

    delta = 0;
    OMP("omp parallel for reduction(+:delta)")
    for (uint64_t v = 0; v < NV; v++) {
      double mydelta = tmp_pr[v] - pr[v];
      if (mydelta < 0)
	mydelta = -mydelta;
      delta += mydelta;
    }

    OMP("omp parallel for")
    for(uint64_t v = 0; v < NV; v++) {
      pr[v] = tmp_pr[v];
    }

    iter--;
  }

  if (!tmp_pr_in)
    free (tmp_pr);
}

int
main(int argc, char *argv[])
{
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  char name[1024];
  if(argc > 1) {
    sprintf(name, "pagerank_%s", argv[1]);
  }

  stinger_registered_alg * alg = 
    stinger_register_alg(
      .name=argc > 1 ? name : "pagerank",
      .data_per_vertex=sizeof(double),
      .data_description="d pagerank",
      .host="localhost",
    );

  if(!alg) {
    LOG_E("Registering algorithm failed.  Exiting");
    return -1;
  }

  double * pr = (double *)alg->alg_data;
  OMP("omp parallel for")
  for(uint64_t v = 0; v < alg->stinger->max_nv; v++) {
    pr[v] = 1 / ((double)alg->stinger->max_nv);
  }

  double * tmp_pr = (double *)xcalloc(alg->stinger->max_nv, sizeof(double));

  double time;
  init_timer();
  
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {
    int64_t type = -1;
    if(argc > 1) {
      type = stinger_etype_names_lookup_type(alg->stinger, argv[1]);
    }
    if(argc > 1 && type > -1) {
      page_rank_type(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, 1e-8, 0.85, 100, type);
    } else if (argc <= 1) {
      page_rank(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, 1e-8, 0.85, 100);
    }
  } stinger_alg_end_init(alg);

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Streaming Phase
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  while(alg->enabled) {
    /* Pre processing */
    if(stinger_alg_begin_pre(alg)) {
      /* nothing to do */
      time = timer();
      stinger_alg_end_pre(alg);
      time = timer() - time;
      LOG_I_A("Pre time : %20.15e", time);
    }

    /* Post processing */
      time = timer();
    if(stinger_alg_begin_post(alg)) {
      int64_t type = -1;
      if(argc > 1) {
	type = stinger_etype_names_lookup_type(alg->stinger, argv[1]);
	if(type > -1) {
	  page_rank_type(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, 1e-8, 0.85, 100, type);
	} else {
	  LOG_W_A("TYPE DOES NOT EXIST %s", argv[1]);
	  LOG_W("Existing types:");
	  for(int64_t t = 0; t < stinger_etype_names_count(alg->stinger); t++) {
	    LOG_W_A("  > %ld %s", (long) t, stinger_etype_names_lookup_name(alg->stinger, t));
	  }
	}
      } else {
	page_rank(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, 1e-8, 0.85, 10);
      }
      stinger_alg_end_post(alg);
    }
      time = timer() - time;
      LOG_I_A("Post time : %20.15e", time);
  }

  LOG_I("Algorithm complete... shutting down");

  free(tmp_pr);
}

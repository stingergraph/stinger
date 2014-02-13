#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"

int64_t
page_rank(stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter)
{
  double * tmp_pr = NULL;
  if(tmp_pr_in) {
    tmp_pr = tmp_pr_in;
  } else {
    tmp_pr = (double *)xmalloc(sizeof(double) * NV);
  }

  int64_t iter = maxiter;
  double delta = 1;

  while(delta > epsilon && iter > 0) {
    OMP("omp parallel for")
    for(uint64_t v = 0; v < NV; v++) {
      tmp_pr[v] = 0;

      STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, v) {
	tmp_pr[v] += (((double)pr[STINGER_EDGE_DEST]) / 
	  ((double) stinger_outdegree(S, STINGER_EDGE_DEST)));
      } STINGER_FORALL_EDGES_OF_VTX_END();
    }

    OMP("omp parallel for")
    for(uint64_t v = 0; v < NV; v++) {
      tmp_pr[v] = tmp_pr[v] * dampingfactor + (((double)(1-dampingfactor)) / ((double)NV));
    }

    delta = 0;
    OMP("omp parallel for reduction(+:delta)")
    for(uint64_t v = 0; v < NV; v++) {
      double mydelta = tmp_pr[v] - pr[v];
      if(mydelta < 0)
	mydelta = -mydelta;
      delta += mydelta;
    }

    OMP("omp parallel for")
    for(uint64_t v = 0; v < NV; v++) {
      pr[v] = tmp_pr[v];
    }
  }

  if(!tmp_pr_in)
    free(tmp_pr);
}

int64_t
page_rank_type(stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter, int64_t type)
{
  double * tmp_pr = NULL;
  if(tmp_pr_in) {
    tmp_pr = tmp_pr_in;
  } else {
    tmp_pr = (double *)xmalloc(sizeof(double) * NV);
  }

  int64_t iter = maxiter;
  double delta = 1;

  while(delta > epsilon && iter > 0) {
    OMP("omp parallel for")
    for(uint64_t v = 0; v < NV; v++) {
      tmp_pr[v] = 0;

      STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(S, type, v) {
	tmp_pr[v] += (((double)pr[STINGER_EDGE_DEST]) / 
	  ((double) stinger_outdegree(S, STINGER_EDGE_DEST)));
      } STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_END();
    }

    OMP("omp parallel for")
    for(uint64_t v = 0; v < NV; v++) {
      tmp_pr[v] = tmp_pr[v] * dampingfactor + (((double)(1-dampingfactor)) / ((double)NV));
    }

    delta = 0;
    OMP("omp parallel for reduction(+:delta)")
    for(uint64_t v = 0; v < NV; v++) {
      double mydelta = tmp_pr[v] - pr[v];
      if(mydelta < 0)
	mydelta = -mydelta;
      delta += mydelta;
    }

    OMP("omp parallel for")
    for(uint64_t v = 0; v < NV; v++) {
      pr[v] = tmp_pr[v];
    }
  }

  if(!tmp_pr_in)
    free(tmp_pr);
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
  for(uint64_t v = 0; v < STINGER_MAX_LVERTICES; v++) {
    pr[v] = 1 / ((double)STINGER_MAX_LVERTICES);
  }

  double * tmp_pr = (double *)xcalloc(STINGER_MAX_LVERTICES, sizeof(double));
  
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
      stinger_alg_end_pre(alg);
    }

    /* Post processing */
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
	page_rank(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, 1e-8, 0.85, 100);
      }
      stinger_alg_end_post(alg);
    }
  }

  LOG_I("Algorithm complete... shutting down");

  free(tmp_pr);
}

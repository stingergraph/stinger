#include <stdio.h>

#include "pagerank.h"
#include "stinger_core/x86_full_empty.h"

inline double * set_tmp_pr(double * tmp_pr_in, int64_t NV) {
  double * tmp_pr = NULL;
  if (tmp_pr_in) {
    tmp_pr = tmp_pr_in;
  } else {
    tmp_pr = (double *)xmalloc(sizeof(double) * NV);
  }
  return tmp_pr;
}

inline void unset_tmp_pr(double * tmp_pr, double * tmp_pr_in) {
  if (!tmp_pr_in)
    free(tmp_pr);
}

int64_t
page_rank_directed(stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter) {
  double * tmp_pr = set_tmp_pr(tmp_pr_in, NV);

  int64_t * pr_lock = (int64_t *)xcalloc(NV,sizeof(double));

  int64_t iter = maxiter;
  double delta = 1;
  int64_t iter_count = 0;

  while (delta > epsilon && iter > 0) {
    iter_count++;

    for (uint64_t v = 0; v < NV; v++) {
      tmp_pr[v] = 0;
    }

    STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_BEGIN(S) {
      int64_t outdegree = stinger_outdegree(S, STINGER_EDGE_SOURCE);
      int64_t count = readfe(&pr_lock[STINGER_EDGE_DEST]);
      tmp_pr[STINGER_EDGE_DEST] += (((double)pr[STINGER_EDGE_SOURCE]) /
        ((double) (outdegree ? outdegree : NV -1)));
      writeef(&pr_lock[STINGER_EDGE_DEST],count+1);
    } STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_END();

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
    //LOG_I_A("delta : %20.15e", delta);

    OMP("omp parallel for")
    for (uint64_t v = 0; v < NV; v++) {
      pr[v] = tmp_pr[v];
    }

    iter--;
  }

  unset_tmp_pr(tmp_pr,tmp_pr_in);
  xfree(pr_lock);
}

int64_t
page_rank (stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter)
{
  double * tmp_pr = set_tmp_pr(tmp_pr_in, NV);

  int64_t iter = maxiter;
  double delta = 1;
  int64_t iter_count = 0;

  while (delta > epsilon && iter > 0) {
    iter_count++;

    OMP("omp parallel for")
    for (uint64_t v = 0; v < NV; v++) {
      tmp_pr[v] = 0;

      STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, v) {
        int64_t outdegree = stinger_outdegree (S, STINGER_EDGE_DEST);
        tmp_pr[v] += (((double) pr[STINGER_EDGE_DEST]) / 
          ((double) (outdegree ? outdegree : NV-1)));
      } STINGER_FORALL_EDGES_OF_VTX_END();
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
    //LOG_I_A("delta : %20.15e", delta);

    OMP("omp parallel for")
    for (uint64_t v = 0; v < NV; v++) {
      pr[v] = tmp_pr[v];
    }

    iter--;
  }

  LOG_I_A("PageRank iteration count : %ld", iter_count);

  unset_tmp_pr(tmp_pr,tmp_pr_in);
}

int64_t
page_rank_type_directed(stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter, int64_t type)
{
  double * tmp_pr = set_tmp_pr(tmp_pr_in, NV);

  int64_t iter = maxiter;
  double delta = 1;
  int64_t iter_count = 0;

  while (delta > epsilon && iter > 0) {
    iter_count++;

    for (uint64_t v = 0; v < NV; v++) {
      tmp_pr[v] = 0;
    }

    STINGER_FORALL_EDGES_BEGIN(S,type) {
      /* TODO: this should be typed outdegree */
      int64_t outdegree = stinger_outdegree(S, STINGER_EDGE_SOURCE);
      tmp_pr[STINGER_EDGE_DEST] += (((double)pr[STINGER_EDGE_SOURCE]) /
        ((double) (outdegree ? outdegree : NV -1)));
    } STINGER_FORALL_EDGES_END();

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
    //LOG_I_A("delta : %20.15e", delta);

    OMP("omp parallel for")
    for (uint64_t v = 0; v < NV; v++) {
      pr[v] = tmp_pr[v];
    }

    iter--;
  }

  unset_tmp_pr(tmp_pr,tmp_pr_in);
}

int64_t
page_rank_type(stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter, int64_t type)
{
  double * tmp_pr = set_tmp_pr(tmp_pr_in, NV);

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

  unset_tmp_pr(tmp_pr,tmp_pr_in);
}
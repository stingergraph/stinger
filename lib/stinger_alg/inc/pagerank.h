#ifndef STINGER_PAGERANK_H_
#define STINGER_PAGERANK_H_

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"

#define EPSILON_DEFAULT 1e-8
#define DAMPINGFACTOR_DEFAULT 0.85
#define MAXITER_DEFAULT 20

void page_rank_subset(stinger_t * S, int64_t NV, uint8_t * vertex_set, int64_t vertex_set_size, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter);
void page_rank_directed(stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter);
void page_rank (stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter);
void page_rank_type(stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter, int64_t type);
void page_rank_type_directed(stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter, int64_t type);


double * set_tmp_pr(double * tmp_pr_in, int64_t NV);
void unset_tmp_pr(double * tmp_pr, double * tmp_pr_in);

#endif

#ifndef STINGER_BETWEENNESS_H_
#define STINGER_BETWEENNESS_H_
#define frac(x,y) ((double)(((double)(x))/((double)(y))))

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"

void single_bc_search(stinger_t * S, int64_t nv, int64_t source, double * bc, int64_t * found_count);
void sample_search(stinger_t * S, int64_t nv, int64_t nsamples, double * bc, int64_t * found_count);

#endif

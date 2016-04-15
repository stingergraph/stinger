#ifndef STINGER_BFS_H_
#define STINGER_BFS_H_

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"

int64_t
parallel_breadth_first_search (
    struct stinger * S,
    int64_t nv,
    int64_t source,
    int64_t * marks,
    int64_t * queue,
    int64_t * Qhead,
    int64_t * level
);

int64_t
direction_optimizing_parallel_breadth_first_search (
    struct stinger * S,
    int64_t nv,
    int64_t source,
    int64_t * marks,
    int64_t * queue,
    int64_t * Qhead,
    int64_t * level
);

#endif

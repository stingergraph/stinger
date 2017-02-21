#ifndef STINGER_GRAPH_PARTITION_H
#define STINGER_GRAPH_PARTITION_H

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"

void graph_partition(struct stinger * S, uint64_t NV, uint64_t num_partitions, double_t partition_capacity, int64_t * partitions, int64_t * partitions_sizes);
#endif

#ifndef STINGER_COMMUNITY_ON_DEMAND_H
#define STINGER_COMMUNITY_ON_DEMAND_H

#include "stinger_core/stinger_atomics.h"
#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

int64_t community_on_demand(const stinger_t * S, int64_t ** vertices, int64_t ** partitions);

#ifdef __cplusplus
}
#endif

#endif

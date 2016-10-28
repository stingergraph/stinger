#ifndef STINGER_DIAMETER_H
#define STINGER_DIAMETER_H

#include "shortest_paths.h"

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <limits>

int64_t pseudo_diameter(stinger_t * S,int64_t NV , int64_t source, int64_t dist, bool ignore_weights);

int64_t exact_diameter(stinger_t * S, int64_t NV);

#endif //DYNOGRAPH_DIAMETER_H

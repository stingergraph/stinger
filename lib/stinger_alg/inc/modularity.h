//
// Created by jdeeb3 on 6/9/16.
//

#ifndef STINGER_MODULARITY_H
#define STINGER_MODULARITY_H

#include <float.h>
#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"

void community_detection(stinger_t * S, int64_t NV, int64_t * partitions, int64_t maxIter);

#endif //DYNOGRAPH_MODULARITY_H

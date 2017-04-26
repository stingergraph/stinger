//
// Created by jdeeb3 on 5/27/16.
//

#ifndef STINGER_HITS_CENTRALITY_H
#define STINGER_HITS_CENTRALITY_H

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "math.h"

void hits_centrality(stinger_t * s, int64_t NV, double_t * hubs_scores, double_t * authority_scores, int64_t k);

#endif //DYNOGRAPH_HITS_CENTRALITY_H

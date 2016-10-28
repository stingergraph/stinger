//
// Created by jdeeb3 on 5/20/16.
//

#ifndef STINGER_SHORTEST_PATHS_H
#define STINGER_SHORTEST_PATHS_H

extern "C" {
#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
}
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <limits>



int64_t a_star(stinger_t * S, int64_t NV, int64_t source_vertex, int64_t dest_vertex, bool ignore_weights);

std::vector<int64_t> dijkstra(stinger_t * S, int64_t NV, int64_t source_vertex, bool ignore_weights);

std::vector<std::vector <int64_t> > all_pairs_dijkstra(stinger_t * S, int64_t NV);

int64_t mean_shortest_path(stinger * S, int64_t NV);

#endif //STINGER_SHORTEST_PATHS_H

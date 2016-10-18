#ifndef STINGER_BATCH_INSERT_H_
#define STINGER_BATCH_INSERT_H_

#include "stinger.h"
// HACK
#include <stinger_alg.h>
//#include <stinger_net/stinger_alg.h>

#include <vector>

void
stinger_batch_update(stinger * G, std::vector<stinger_edge_update> &updates, int64_t direction, int64_t operation);

void
stinger_update_directed_edges_for_vertex(
        stinger *G, int64_t src, int64_t type,
        std::vector<stinger_edge_update>::iterator updates_begin,
        std::vector<stinger_edge_update>::iterator updates_end,
        int64_t direction, int64_t operation);

#endif //STINGER_BATCH_INSERT_H_

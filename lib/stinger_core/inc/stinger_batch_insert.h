#ifndef STINGER_BATCH_INSERT_H_
#define STINGER_BATCH_INSERT_H_

#include "stinger.h"
// HACK
#include <stinger_net/stinger_alg.h>
//#include <stinger_net/stinger_alg.h>

#include <vector>

void
stinger_batch_incr_edge(stinger * G, std::vector<stinger_edge_update> &updates);
void
stinger_batch_insert_edge(stinger * G, std::vector<stinger_edge_update> &updates);
void
stinger_batch_incr_edge_pair(stinger * G, std::vector<stinger_edge_update> &updates);
void
stinger_batch_incr_edge_pair(stinger * G, std::vector<stinger_edge_update> &updates);

#endif //STINGER_BATCH_INSERT_H_

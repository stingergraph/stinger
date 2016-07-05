#ifndef STINGER_STREAMING_CONNECTED_COMPONENTS_H_
#define STINGER_STREAMING_CONNECTED_COMPONENTS_H_
#include <stdint.h>

#include "stinger_core/stinger_atomics.h"
#include "stinger_core/x86_full_empty.h"
#include "stinger_utils/stinger_utils.h"
#include "stinger_core/stinger.h"
#include "stinger_utils/timer.h"
#include "stinger_core/xmalloc.h"

typedef struct stinger_connected_components_stats stinger_connected_components_stats;

void stinger_scc_reset_stats(stinger_connected_components_stats* stats);
void stinger_scc_print_insert_stats(stinger_connected_components_stats* stats);
void stinger_scc_print_delete_stats(stinger_connected_components_stats* stats);

typedef struct stinger_scc_internal stinger_scc_internal; 


// Recommended default for (parentsPerVertex==4).
void stinger_scc_initialize_internals(struct stinger * S, int64_t nv, stinger_scc_internal* scc_internal, int64_t parentsPerVertex);

void stinger_scc_release_internals(stinger_scc_internal* scc_internal);


#endif

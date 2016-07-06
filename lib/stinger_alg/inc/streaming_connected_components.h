#ifndef STINGER_STREAMING_CONNECTED_COMPONENTS_H_
#define STINGER_STREAMING_CONNECTED_COMPONENTS_H_
#include <stdint.h>

#include "stinger_core/stinger_atomics.h"
#include "stinger_core/x86_full_empty.h"
#include "stinger_utils/stinger_utils.h"
#include "stinger_core/stinger.h"
#include "stinger_utils/timer.h"
#include "stinger_core/xmalloc.h"

typedef struct {
	uint64_t bfs_deletes_in_tree;
	uint64_t bfs_inserts_in_tree_as_parents;
	uint64_t bfs_inserts_in_tree_as_neighbors;
	uint64_t bfs_inserts_in_tree_as_replacement;
	uint64_t bfs_inserts_bridged;
	uint64_t bfs_unsafe_deletes;
}stinger_connected_components_stats;

void stinger_scc_print_insert_stats(stinger_connected_components_stats* stats);
void stinger_scc_print_delete_stats(stinger_connected_components_stats* stats);

typedef struct{
	int64_t * bfsDataPtr;
	int64_t * queue;
	int64_t * level;
	int64_t * found;
	int64_t * same_level_queue;  

	int64_t * parentsDataPtr;
	int64_t * parentArray;
	int64_t * parentCounter;
	int64_t * bfs_components;
	int64_t * bfs_component_sizes;

	int64_t   parentsPerVertex;
	int64_t   initCCCount;
} stinger_scc_internal;


// Needs to be called once before the updates are processed. 
// This function uses a "static graph" algorithm to initialize the data
// structure that is used for the streaming updates.
// Recommended default for (parentsPerVertex==4).
void stinger_scc_initialize_internals(struct stinger * S, int64_t nv, stinger_scc_internal* scc_internal, int64_t parentsPerVertex);
void stinger_scc_release_internals(stinger_scc_internal* scc_internal);

// Should be called before each batch update.
void stinger_scc_reset_stats(stinger_connected_components_stats* stats);

// Update the streaming connected components with a batch of updates.
int stinger_scc_update(struct stinger * S, int64_t nv,  stinger_scc_internal scc_internal, 
	stinger_connected_components_stats* stats, int64_t *batch,int64_t batch_size);

// Gets the connected component mapping of the dynamic graph algorithm.
const int64_t* stinger_scc_get_components(stinger_scc_internal scc_internal);

#endif

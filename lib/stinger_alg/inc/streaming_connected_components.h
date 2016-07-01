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
	uint64_t bfs_real_deletes;
	uint64_t bfs_real_inserts;
	uint64_t bfs_total_deletes;
	uint64_t bfs_total_inserts;
	uint64_t bfs_unsafe_deletes;
}stinger_connected_components_stats;

void stinger_scc_reset_stats(stinger_connected_components_stats* stats){
	stats->bfs_deletes_in_tree = 0;
	stats->bfs_inserts_in_tree_as_parents = 0;
	stats->bfs_inserts_in_tree_as_neighbors = 0;
	stats->bfs_inserts_in_tree_as_replacement = 0;
	stats->bfs_inserts_bridged = 0;
	stats->bfs_real_deletes = 0;
	stats->bfs_real_inserts = 0;
	stats->bfs_total_deletes = 0;
	stats->bfs_total_inserts = 0;
	stats->bfs_unsafe_deletes = 0;
}
void stinger_scc_print_insert_stats(stinger_connected_components_stats* stats){
	printf("bfs_inserts_in_tree_as_parents %ld", stats->bfs_inserts_in_tree_as_parents);
	printf("bfs_inserts_in_tree_as_neighbors %ld", stats->bfs_inserts_in_tree_as_neighbors);
	printf("bfs_inserts_in_tree_as_replacement  %ld", stats->bfs_inserts_in_tree_as_replacement);
	printf("bfs_inserts_bridged  %ld", stats->bfs_inserts_bridged);
	printf("bfs_real_inserts  %ld", stats->bfs_real_inserts);
	printf("bfs_total_inserts  %ld", stats->bfs_total_inserts);	
}

void stinger_scc_print_delete_stats(stinger_connected_components_stats* stats){
	printf("bfs_deletes_in_tree %ld", stats->bfs_deletes_in_tree);
	printf("bfs_real_deletes %ld", stats->bfs_real_deletes);
	printf("bfs_total_deletes %ld", stats->bfs_total_deletes);
	printf("bfs_unsafe_deletes %ld", stats->bfs_unsafe_deletes);
}


#endif

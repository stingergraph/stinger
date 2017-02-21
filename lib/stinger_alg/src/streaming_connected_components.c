
#include <stdio.h>
#include "streaming_connected_components.h"
#include "weakly_connected_components.h"

#define LEVEL_IS_NEG(k) ((level[(k)] < 0) && (level[(k)] != INFINITY_MY))
#define LEVEL_IS_POS(k) ((level[(k)] >= 0) && (level[(k)] != INFINITY_MY))
#define LEVEL_IS_INF(k) (level[(k)] == INFINITY_MY)
#define LEVEL_EQUALS(k,y) ((level[(k)] == (y)) && (level[(k)] != INFINITY_MY))
#define SWAP_UINT64(x,y) {uint64_t tmp = (x); (x) = (y); (y) = tmp;}

#define INFINITY_MY INT64_MAX>>2
#define EMPTY_NEIGHBOR INT64_MIN>>2
// #define INFINITY_MY 1073741824
// #define EMPTY_NEIGHBOR -1073741824


void stinger_scc_reset_stats(stinger_connected_components_stats* stats){
	stats->bfs_deletes_in_tree = 0;
	stats->bfs_inserts_in_tree_as_parents = 0;
	stats->bfs_inserts_in_tree_as_neighbors = 0;
	stats->bfs_inserts_in_tree_as_replacement = 0;
	stats->bfs_inserts_bridged = 0;
	stats->bfs_unsafe_deletes = 0;
}
void stinger_scc_print_insert_stats(stinger_connected_components_stats* stats){
	printf("bfs_inserts_in_tree_as_parents %ld\n", stats->bfs_inserts_in_tree_as_parents);
	printf("bfs_inserts_in_tree_as_neighbors %ld\n", stats->bfs_inserts_in_tree_as_neighbors);
	printf("bfs_inserts_in_tree_as_replacement  %ld\n", stats->bfs_inserts_in_tree_as_replacement);
	printf("bfs_inserts_bridged  %ld\n", stats->bfs_inserts_bridged);
}

void stinger_scc_print_delete_stats(stinger_connected_components_stats* stats){
	printf("bfs_deletes_in_tree %ld\n", stats->bfs_deletes_in_tree);
	printf("bfs_unsafe_deletes %ld\n", stats->bfs_unsafe_deletes);
}

const int64_t* stinger_scc_get_components(stinger_scc_internal scc_internal){
	return scc_internal.bfs_components;
}

void stinger_scc_copy_component_array(stinger_scc_internal scc_internal,int64_t* destArray){
	memcpy(destArray, scc_internal.bfs_components, sizeof(int64_t)*scc_internal.nv);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * inline function definitions
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

inline void update_tree_for_delete_directed (int64_t * parentArray, int64_t * parentCounter, 
	int64_t * level, int64_t parentsPerVertex, int64_t i, int64_t j,stinger_connected_components_stats* stats);

inline int64_t update_tree_for_insert_directed (int64_t * parentArray, int64_t * parentCounter, 
	int64_t * level,int64_t * component,int64_t parentsPerVertex, int64_t i, int64_t j,stinger_connected_components_stats* stats);

inline int64_t is_delete_unsafe (int64_t * parentArray, int64_t * parentCounter, 
	int64_t * level,int64_t parentsPerVertex, int64_t i,stinger_connected_components_stats* stats); 

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * bfs connected components functions
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

uint64_t bfs_build_component (struct stinger* S, uint64_t currRoot, uint64_t* queue, 
	uint64_t* level, uint64_t* parentArray, uint64_t parentsPerVertex, uint64_t* parentCounter, 
	int64_t * component)
{
  component[currRoot] = currRoot;
  level[currRoot] = 0;

  queue[0] = currRoot;
  int64_t qStart  = 0; 
  int64_t qEnd	  = 1;

  uint64_t depth = 0;

  /* while queue is not empty */
  	while(qStart != qEnd) {
		uint64_t old_qEnd = qEnd;

		depth++;

		OMP("omp parallel for")
		for(int64_t i = qStart; i < old_qEnd; i++) {
			uint64_t currElement = queue[i];
			uint64_t myLevel = level[currElement];
			uint64_t nextLevel = myLevel+1;

			STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, currElement) {
			  uint64_t k = STINGER_EDGE_DEST;

			  /* if k hasn't been found */
			  if(LEVEL_IS_INF(k)) {
				/* add k to the frontier */
				if(INFINITY_MY == stinger_int64_cas(level + k, INFINITY_MY, nextLevel)) {
				  uint64_t which = stinger_int64_fetch_add(&qEnd, 1);
				  queue[which] = k;
				  component[k] = currRoot;
				}
			  }

			  /* if k has space */
			  if(parentCounter[k] < parentsPerVertex) {
				if(LEVEL_EQUALS(k,nextLevel)) {
				  uint64_t which = stinger_int64_fetch_add(parentCounter + k, 1);
				  if(which < parentsPerVertex) {
					/* add me to k's parents */
					parentArray[k*parentsPerVertex+which] = currElement;
				  } else {
					parentCounter[k] = parentsPerVertex;
				  }
				} else if(LEVEL_EQUALS(k, myLevel)) {
				  uint64_t which = stinger_int64_fetch_add(parentCounter + k, 1);
				  if(which < parentsPerVertex) {
					/* add me to k as a neighbor (bitwise negate for vtx 0) */
					parentArray[k*parentsPerVertex+which] = ~currElement;
				  } else {
					parentCounter[k] = parentsPerVertex;
				  }
				}
			  }
			} STINGER_FORALL_EDGES_OF_VTX_END();
		}
		qStart = old_qEnd;
  	}

  return qEnd;
}

uint64_t bfs_build_new_component (struct stinger* S, uint64_t currRoot, 
	uint64_t component_id, int64_t start_level, uint64_t* queue,uint64_t* level, 
	uint64_t* parentArray, uint64_t parentsPerVertex, uint64_t* parentCounter, int64_t * component)
{
  component[currRoot] = component_id;
  level[currRoot] = start_level;

  queue[0] = currRoot;
  int64_t qStart  = 0; 
  int64_t qEnd	  = 1;

  uint64_t depth = 0;

  /* while queue is not empty */
  while(qStart != qEnd) {
	uint64_t old_qEnd = qEnd;

	depth++;

	OMP("omp parallel for")
	  for(int64_t i = qStart; i < old_qEnd; i++) {
		uint64_t currElement = queue[i];
		int64_t myLevel = level[currElement];
		int64_t nextLevel = myLevel+1;

		STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, currElement) {
		  uint64_t k = STINGER_EDGE_DEST;

		  /* if k hasn't been found */
		  if(component[k] != component_id) {
			/* local level */
			int64_t k_level = readfe(level + k);
			if(component[k] != component_id) {
			  uint64_t which = stinger_int64_fetch_add(&qEnd, 1);
			  queue[which] = k;
			  component[k] = component_id;
			  parentCounter[k] = 0;
			  k_level = nextLevel;
			}
			writeef(level + k, k_level);
		  }

		  /* if k has space */
		  if(parentCounter[k] < parentsPerVertex) {
			if(readff(level + k) == nextLevel) {
			  uint64_t which = stinger_int64_fetch_add(parentCounter + k, 1);
			  if(which < parentsPerVertex) {
				/* add me to k's parents */
				parentArray[k*parentsPerVertex+which] = currElement;
			  } else {
				parentCounter[k] = parentsPerVertex;
			  }
			} else if(readff(level+k) == myLevel) {
			  uint64_t which = stinger_int64_fetch_add(parentCounter + k, 1);
			  if(which < parentsPerVertex) {
				/* add me to k as a neighbor (bitwise negate for vtx 0) */
				parentArray[k*parentsPerVertex+which] = ~currElement;
			  } else {
				parentCounter[k] = parentsPerVertex;
			  }
			}
		  }
		} STINGER_FORALL_EDGES_OF_VTX_END();
	  }
	qStart = old_qEnd;
  }

  // char component_name[256];
  // sprintf(component_name, "connection_depth[%ld]", component_id);
  // CC_STAT_INT64(component_name, depth);

  return qEnd;
}

#define LDB(...) //printf(__VA_ARGS__);

/** @brief Fix a component after an unsafe deletion. 
 * Attempts to find a path back up the tree (to a vertex that can reach a 
 * higher level). If one is found, the tree is rebuilt downward from there.
 * Otherwise, this function also builds a new component along the way.
 **/
uint64_t bfs_rebuild_component (struct stinger* S, uint64_t currRoot, uint64_t component_id, 
	int64_t start_level, uint64_t* queue,uint64_t* level, uint64_t* parentArray, 
	uint64_t parentsPerVertex, uint64_t* parentCounter, int64_t * component,int64_t * start_level_queue)
{
  int64_t old_component = component[currRoot];
  component[currRoot] = component_id;
  level[currRoot] = 0;
  parentCounter[currRoot] = 0;

  queue[0] = currRoot;
  int64_t qStart    = 0; 
  int64_t qEnd	    = 1;

  int64_t slq_start = 0;
  int64_t slq_end   = 0;

  int path_up_found = 0;

  /* while queue is not empty */
  while(qStart != qEnd) {
	uint64_t old_qEnd = qEnd;

	OMP("omp parallel for")
	  for(int64_t i = qStart; i < old_qEnd; i++) {
		uint64_t currElement = queue[i];
		int64_t myLevel = level[currElement];
		int64_t nextLevel = myLevel+1;

		STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, currElement) {
		  uint64_t k = STINGER_EDGE_DEST;

		  /* if k hasn't been found */
		  if(component[k] != component_id) {
			/* local level */
			int64_t k_level = readfe(level + k);
			if(component[k] != component_id) {
			  if(k_level == start_level) {
				LDB("\n\tFOUND ROOT: %ld", k);
				component[k] = component_id;
				path_up_found = 1;
				uint64_t which = stinger_int64_fetch_add(&slq_end, 1);
				start_level_queue[which] = k;
			  } else {
				if((k_level < start_level) && (k_level > ~start_level)) {
				  LDB("\n\tFOUND ROOT: %ld", k);
				  component[k] = component_id;
				  path_up_found = 1;
				  uint64_t which = stinger_int64_fetch_add(&slq_end, 1);
				  start_level_queue[which] = k;
				} else {
				  LDB("\n\tFOUND OTHER: %ld", k);
				  parentCounter[k] = 0;
				  uint64_t which = stinger_int64_fetch_add(&qEnd, 1);
				  queue[which] = k;
				  component[k] = component_id;
				  k_level = nextLevel;
				}
			  }
			}
			writeef(level + k, k_level);
		  }

		  /* if k has space */
		  if(parentCounter[k] < parentsPerVertex) {
			if(readff(level + k) == nextLevel) {
			  uint64_t which = stinger_int64_fetch_add(parentCounter + k, 1);
			  if(which < parentsPerVertex) {
				/* add me to k's parents */
				parentArray[k*parentsPerVertex+which] = currElement;
			  } else {
				parentCounter[k] = parentsPerVertex;
			  }
			} else if(readff(level+k) == myLevel) {
			  uint64_t which = stinger_int64_fetch_add(parentCounter + k, 1);
			  if(which < parentsPerVertex) {
				/* add me to k as a neighbor (bitwise negate for vtx 0) */
				parentArray[k*parentsPerVertex+which] = ~currElement;
			  } else {
				parentCounter[k] = parentsPerVertex;
			  }
			}
		  }
		} STINGER_FORALL_EDGES_OF_VTX_END();
	  }
	qStart = old_qEnd;
  }

  if(!path_up_found) {
	return qEnd;
  } else {

	OMP("omp parallel for")
	  for(int64_t i = slq_start; i < slq_end; i++) {
		int64_t currElement = component[start_level_queue[i]] = old_component;
	  }

	while(slq_end != slq_start) {
	  /* rebuilt from the current level down */
	  uint64_t old_slq_end= slq_end;

	  OMP("omp parallel for")
		for(int64_t i = slq_start; i < old_slq_end; i++) {
		  int64_t currElement = start_level_queue[i];
		  int64_t myLevel = level[currElement];

		  if(myLevel < 0)
			myLevel = ~myLevel;

		  int64_t nextLevel = myLevel+1;

		  LDB("\n\tSTARTING FROM: %ld", currElement);

		  STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, currElement) {
			uint64_t k = STINGER_EDGE_DEST;

			/* if k hasn't been found */
			if(component[k] != old_component) {
			  /* local level */
			  int64_t k_level = readfe(level + k);
			  if(component[k] != old_component) {
				parentCounter[k] = 0;
				uint64_t which = stinger_int64_fetch_add(&slq_end, 1);
				start_level_queue[which] = k;
				component[k] = old_component;
				k_level = nextLevel;
			  }
			  writeef(level + k, k_level);
			}

			/* if k has space */
			if(parentCounter[k] < parentsPerVertex) {
			  if(readff(level + k) == nextLevel) {
				uint64_t which = stinger_int64_fetch_add(parentCounter + k, 1);
				if(which < parentsPerVertex) {
				  /* add me to k's parents */
				  parentArray[k*parentsPerVertex+which] = currElement;
				} else {
				  parentCounter[k] = parentsPerVertex;
				}
			  } else if(readff(level+k) == myLevel) {
				uint64_t which = stinger_int64_fetch_add(parentCounter + k, 1);
				if(which < parentsPerVertex) {
				  /* add me to k as a neighbor (bitwise negate for vtx 0) */
				  parentArray[k*parentsPerVertex+which] = ~currElement;
				} else {
				  parentCounter[k] = parentsPerVertex;
				}
			  }
			}
		  } STINGER_FORALL_EDGES_OF_VTX_END();
		}
	  slq_start = old_slq_end;
	}
  }
}

void update_tree_for_delete_directed (int64_t * parentArray, int64_t * parentCounter, 
	int64_t * level,int64_t parentsPerVertex, int64_t i, int64_t j,stinger_connected_components_stats* stats) {
  int64_t local_parent_counter = readfe((uint64_t *)(parentCounter + i));
  int i_parents = 0;
  for(int64_t p = 0; p < local_parent_counter; p++) {
	/* if j is a neighbor or parent of i */
	if(parentArray[(i)*parentsPerVertex+p]==(j) ||
		parentArray[(i)*parentsPerVertex+p]==(~j)) {
	  /* replace with last element */
	  local_parent_counter--;
	  parentArray[(i)*parentsPerVertex + p] = parentArray[(i)*parentsPerVertex + local_parent_counter];
	  parentArray[(i)*parentsPerVertex + local_parent_counter] = EMPTY_NEIGHBOR;
	  stinger_int64_fetch_add(&stats->bfs_deletes_in_tree, 1);
	}
	/* also search to see if we still have parents */
	if(parentArray[(i)*parentsPerVertex+p] >= 0) {
	  i_parents = 1;
	}
  }
  if(!i_parents && LEVEL_IS_POS(i)) {
	level[i] = ~level[i];
  }
  writeef((uint64_t *)(parentCounter + i), local_parent_counter);
}

int64_t update_tree_for_insert_directed (int64_t * parentArray, int64_t * parentCounter, 
	int64_t * level,int64_t * component,int64_t parentsPerVertex, int64_t i, int64_t j,stinger_connected_components_stats* stats) {
  if(component[i] == component[j]) {
	int64_t local_parent_counter = readfe((uint64_t *)(parentCounter + j));
	if(LEVEL_IS_NEG(j)) {
	  if(LEVEL_IS_POS(i) && level[i] < ~level[j]) {
		if(local_parent_counter < parentsPerVertex) {
		  parentArray[j*parentsPerVertex + local_parent_counter] = i;
		  local_parent_counter++;
		  // stinger_int64_fetch_add(&stats->bfs_inserts_in_tree_as_parents, 1);
		} else {
		  parentArray[j*parentsPerVertex] = i;
		}
		level[j] = ~level[j];
	  } 
	}
	 else if(LEVEL_IS_POS(i)) {
	  if(local_parent_counter < parentsPerVertex) {
		if(level[i] < level[j]) {
		  parentArray[j*parentsPerVertex + local_parent_counter] = i;
		  local_parent_counter++;
		  stinger_int64_fetch_add(&stats->bfs_inserts_in_tree_as_parents, 1);
		} else if(level[i] == level[j]) {
		  parentArray[j*parentsPerVertex + local_parent_counter] = ~i;
		  local_parent_counter++;
		  stinger_int64_fetch_add(&stats->bfs_inserts_in_tree_as_neighbors, 1);
		}
	  } else if(level[i] < level[j]) {
		/* search backward - more likely to have neighbors at end */
		for(int64_t p = parentsPerVertex - 1; p >= 0; p--) {
		  if(parentArray[j*parentsPerVertex + p] < 0) {
			parentArray[j*parentsPerVertex + p] = i;
			stinger_int64_fetch_add(&stats->bfs_inserts_in_tree_as_replacement, 1);
			break;
		  }
		}
	  }
	}
	writeef((uint64_t *)(parentCounter + j), local_parent_counter);
	return 0;
  } else {
	return 1;
  }
}

int64_t is_delete_unsafe (int64_t * parentArray, int64_t * parentCounter, 
	int64_t * level,int64_t parentsPerVertex, int64_t i,stinger_connected_components_stats* stats){
  int i_parents = 0;
  int i_neighbors = 0;
  for(int p = 0; p < parentCounter[i]; p++) {
	if(parentArray[(i)*parentsPerVertex+p] >= 0) {
	  i_parents = 1;
	} else if(level[~parentArray[(i)*parentsPerVertex+p]] >= 0) {
	  i_neighbors= 1;
	}
  }
  if(!i_parents) {
	/* if no parents, I isn't safe */
	if(level[i] >= 0) {
	  fprintf(stderr,"\n\t%s %d You shouldn't see this run.", __func__, __LINE__);
	  level[i] = ~level[i];
	}
	/* if no safe neighbors this delete is dangerous */
	if(!i_neighbors) {
	  return 1;
	}
  }
  return 0;
}

int is_tree_wrong (int64_t nv, int64_t * parentArray, int64_t * parentCounter, int64_t parentsPerVertex, 
	int64_t i, int64_t j,const char * msg){ 
  for(int p=0; p<parentCounter[i];p++) {
	int64_t neighbor = parentArray[(i)*parentsPerVertex+p];
	if(neighbor == EMPTY_NEIGHBOR || !(neighbor < nv && neighbor > ~nv)) {
	  printf("\n\tParents %ld Invalid Action Delete %ld %s\n", i, j, msg);
	  return 1;
	}
  }
  return 0;
}

int is_full_tree_wrong (struct stinger * S,int64_t nv,int64_t * parentArray, 
	int64_t * parentCounter, int64_t * level,int64_t parentsPerVertex){ 
  int error = 0;
  for(uint64_t v = 0; v < nv; v++) {
	int i_parents = 0;
	for(int p=0; p<parentCounter[v];p++) {
	  int64_t neighbor = parentArray[(v)*parentsPerVertex+p];
	  if(parentArray[(v)*parentsPerVertex+p] >= 0) 
		i_parents = 1;
	  if(neighbor == EMPTY_NEIGHBOR || !(neighbor < nv && neighbor > ~nv)) {
		printf("\n\t\t** T !RANGE %ld %ld", v, neighbor);
		error = 1;
	  } else {
		if(neighbor < 0)
		  neighbor = ~neighbor;
		if(!stinger_has_typed_successor(S, 0, v, neighbor)) {
		  printf("\n\t\t** T !EXIST %ld %ld", v, neighbor);
		  error = 1;
		}
	  }
	}
	if(i_parents && level[v] < 0) {
	  printf("\n\t\t** T !PARENTS %ld ~%ld (%ld)", v, ~level[v], level[v]);
	  error = 1;
	}
	if((!i_parents) && level[v] > 0) {
	  printf("\n\t\t** T !PARENTS %ld %ld", v, level[v]);
	  error = 1;
	}
  }
  return error;
}

int64_t bfs_component_stats (uint64_t nv, int64_t * sizes, int64_t previous_num) {
#define histogram_max 128

  int64_t min = INT64_MAX;
  int64_t max = INT64_MIN;
  int64_t num_components = 0;
  int64_t size_histogram[histogram_max] = {0};
  double  avg = 0;

  for(uint64_t i = 0; i < nv; i++) {
	int64_t c_size = sizes[i];
	if(c_size) {
	  num_components++;
	  avg += c_size;
	  if(c_size < min)
		min = c_size;
	  if(c_size > max)
		max = c_size;
	  if(c_size < histogram_max)
		size_histogram[c_size]++;
	  else
		size_histogram[histogram_max - 1]++;
	}
  }

  avg /= (double)num_components;

  printf("bfs_component_stats_max %ld\n", max);
  printf("bfs_component_stats_min %ld\n", min);
  printf("bfs_component_stats_avg %lf\n", avg);
  printf("bfs_component_stats_num %ld\n", num_components);

  if(previous_num < num_components) {
	printf("bfs_component_created %ld\n", num_components - previous_num);
  } else {
	printf("bfs_component_joined %ld\n", previous_num - num_components);
  }

  return num_components;
}

int stinger_scc_insertion(struct stinger * S, int64_t nv,  stinger_scc_internal scc_internal, 
	stinger_connected_components_stats* stats, stinger_edge_update* batch,int64_t batch_size){

	// stinger_connected_components_stats* stats, int64_t *batch,int64_t batch_size){
  	/* Updates */
	int64_t * action_stack = malloc(sizeof(int64_t) * batch_size * 2 * 2);
	int64_t * action_stack_components = malloc(sizeof(int64_t) * batch_size * 2 * 2);
	int64_t delete_stack_top;
	int64_t insert_stack_top;

	delete_stack_top = 0;
	insert_stack_top = batch_size * 2 * 2 - 2;

	// int64_t* action = batch;
	// int64_t *actionStream = &action[0];
	int64_t numActions = batch_size;

	int64_t bfs_inserts_bridged=0;

	/* parallel for all insertions */
	OMP("omp parallel for reduction(+:bfs_inserts_bridged)")
	for (int64_t k = 0; k < numActions; k++) {
		int64_t i = batch[k].source;
		int64_t j = batch[k].destination;

		/* if not self-loop */
		if(i != j) {
		  /* if a new edge in stinger, update parents */
			if(update_tree_for_insert_directed(scc_internal.parentArray, scc_internal.parentCounter, 
				scc_internal.level, scc_internal.bfs_components, scc_internal.parentsPerVertex, i, j,stats)) {
			  bfs_inserts_bridged++;
			  /* if component ids are not the same       *
			   * tree update not handled, push onto queue*/
			  int64_t which = stinger_int64_fetch_add(&insert_stack_top, -2);
			  action_stack[which] = i;
			  action_stack[which+1] = j;
			}
		  /* same but for reverse edge j,i */
			if(update_tree_for_insert_directed(scc_internal.parentArray, scc_internal.parentCounter, 
				scc_internal.level, scc_internal.bfs_components, scc_internal.parentsPerVertex, j, i,stats)) {
			  bfs_inserts_bridged++;
			  int64_t which = stinger_int64_fetch_add(&insert_stack_top, -2);
			  action_stack[which] = j;
			  action_stack[which+1] = i;
			}
		}
	} 
	stats->bfs_inserts_bridged = bfs_inserts_bridged;

	/* serial for-all insertions that joined components */
	for(int64_t k = batch_size * 2 * 2 - 2; k > insert_stack_top; k -= 2) {
	  int64_t i = action_stack[k]; 
	  int64_t j = action_stack[k+1];

	  int64_t Ci = scc_internal.bfs_components[i];
	  int64_t Cj = scc_internal.bfs_components[j];

	  if(Ci == Cj)
		continue;

	  int64_t Ci_size = scc_internal.bfs_component_sizes[Ci];
	  int64_t Cj_size = scc_internal.bfs_component_sizes[Cj];

	  if(Ci_size > Cj_size) {
		SWAP_UINT64(i,j)
		SWAP_UINT64(Ci, Cj)
		SWAP_UINT64(Ci_size, Cj_size)
	  }

	  scc_internal.parentArray[i*scc_internal.parentsPerVertex] = j;
	  scc_internal.parentCounter[i] = 1;
	  /* handle singleton */
	  if(Ci_size == 1) {
		scc_internal.bfs_component_sizes[Ci] = 0;
		scc_internal.bfs_component_sizes[Cj]++;
		scc_internal.bfs_components[i] = Cj;
	  } else {
		scc_internal.bfs_component_sizes[Cj] += bfs_build_new_component(S, i, Cj, 
			(scc_internal.level[j] >= 0 ? scc_internal.level[j] : ~scc_internal.level[j])+1, 
			scc_internal.queue, scc_internal.level, scc_internal.parentArray, scc_internal.parentsPerVertex, 
			scc_internal.parentCounter, scc_internal.bfs_components);
		scc_internal.bfs_component_sizes[Ci] = 0;
	  }
	}

	free(action_stack);
	free(action_stack_components);
}

int stinger_scc_deletion(struct stinger * S, int64_t nv,  stinger_scc_internal scc_internal, 
	stinger_connected_components_stats* stats, stinger_edge_update* batch,int64_t batch_size){

	// stinger_connected_components_stats* stats, int64_t *batch,int64_t batch_size){
  	/* Updates */
	int64_t * action_stack = malloc(sizeof(int64_t) * batch_size * 2 * 2);
	int64_t * action_stack_components = malloc(sizeof(int64_t) * batch_size * 2 * 2);
	int64_t delete_stack_top;
	int64_t insert_stack_top;

	delete_stack_top = 0;
	insert_stack_top = batch_size * 2 * 2 - 2;

	// int64_t* action = batch;
	// int64_t *actionStream = &action[0];
	int64_t numActions = batch_size;

	int64_t bfs_inserts_bridged=0;


	/* Parallel forall deletions */
	OMP("omp parallel for")
	for (int64_t k = 0; k < numActions; k++) {
		int64_t i = batch[k].source;
		int64_t j = batch[k].destination;
		/* if not a self-loop */
		if(i != j) {
		  /* for contenience */
		  // i = ~i;
		  // j = ~j;
		  /* if a real edge in stinger, update parents, push onto queue to check safety later */
			uint64_t which = stinger_int64_fetch_add(&delete_stack_top, 2);
			action_stack[which] = i;
			action_stack[which+1] = j;
			action_stack_components[which] = scc_internal.bfs_components[i];
			action_stack_components[which+1] = scc_internal.bfs_components[j];
			update_tree_for_delete_directed(scc_internal.parentArray, scc_internal.parentCounter, 
				scc_internal.level, scc_internal.parentsPerVertex, i, j,stats);

		  /* if a real edge in stinger, update parents, push onto queue to check safety later */
			which = stinger_int64_fetch_add(&delete_stack_top, 2);
			action_stack[which] = j;
			action_stack[which+1] = i;
			action_stack_components[which] = scc_internal.bfs_components[j];
			action_stack_components[which+1] = scc_internal.bfs_components[i];
			update_tree_for_delete_directed(scc_internal.parentArray, scc_internal.parentCounter, 
				scc_internal.level, scc_internal.parentsPerVertex, j, i,stats);
		} 
	  }

	int64_t bfs_unsafe_deletes=0;
	/* parallel safety check */
	OMP("omp parallel for reduction(+:bfs_unsafe_deletes)")
	  for(uint64_t k = 0; k < delete_stack_top; k += 2) {
		int64_t i = action_stack[k];
		int64_t j = action_stack[k+1];
		if(!(is_delete_unsafe(scc_internal.parentArray, scc_internal.parentCounter, 
			scc_internal.level, scc_internal.parentsPerVertex, i,stats))) {
		  action_stack[k] = -1;
		  action_stack[k+1] = -1;
		}
		else bfs_unsafe_deletes += 2;
	}

	stats->bfs_unsafe_deletes = bfs_unsafe_deletes;

	/* explicitly not parallel for all unsafe deletes */
	for(uint64_t k = 0; k < delete_stack_top; k += 2) {
	  int64_t i = action_stack[k];
	  int64_t j = action_stack[k+1];
	  int64_t Ci_prev = action_stack_components[k];
	  int64_t Cj_prev = action_stack_components[k+1];
	  if(i != -1)  {
		int64_t Ci = scc_internal.bfs_components[i];
		int64_t Cj = scc_internal.bfs_components[j];

		if(Ci == Cj && Ci == Ci_prev) {
		  if(Ci != i && stinger_outdegree(S, i) == 0) {
			scc_internal.bfs_component_sizes[Ci]--;
			scc_internal.bfs_components[i] = i;
			scc_internal.bfs_component_sizes[i] = 1;
		  } else if(Cj != j && stinger_outdegree(S, j) == 0) {
			scc_internal.bfs_component_sizes[Cj]--;
			scc_internal.bfs_components[j] = j;
			scc_internal.bfs_component_sizes[j] = 1;
		  } else {
			int64_t level_i = scc_internal.level[i];
			int64_t level_j = scc_internal.level[j];

			if(level_i < 0)
			  level_i = ~level_i;
			if(level_j < 0)
			  level_j = ~level_j;

			if(level_i > level_j) {
			  if(is_delete_unsafe(scc_internal.parentArray, scc_internal.parentCounter, 
			  	scc_internal.level, scc_internal.parentsPerVertex, i,stats)) {
				scc_internal.parentCounter[i] = 0;
				scc_internal.bfs_component_sizes[i] = bfs_rebuild_component(S, i, i, 
					(scc_internal.level[i] >= 0 ? scc_internal.level[i] : ~scc_internal.level[i]), 
					scc_internal.queue, scc_internal.level, scc_internal.parentArray, scc_internal.parentsPerVertex, 
					scc_internal.parentCounter, scc_internal.bfs_components, scc_internal.same_level_queue);
				if(scc_internal.bfs_components[i] != i) {
				  scc_internal.bfs_component_sizes[i] = 0;
				} else {
				  scc_internal.bfs_component_sizes[Cj] -= scc_internal.bfs_component_sizes[i];
				}
			  }
			} else {
			  if(is_delete_unsafe(scc_internal.parentArray, scc_internal.parentCounter, 
			  	scc_internal.level, scc_internal.parentsPerVertex, j,stats)) {
				scc_internal.parentCounter[j] = 0;
				scc_internal.bfs_component_sizes[j] = bfs_rebuild_component(S, j, j, 
					(scc_internal.level[j] >= 0 ? scc_internal.level[j] : ~scc_internal.level[j]), 
					scc_internal.queue, scc_internal.level, scc_internal.parentArray, scc_internal.parentsPerVertex, 
					scc_internal.parentCounter, scc_internal.bfs_components, scc_internal.same_level_queue);
				if(scc_internal.bfs_components[j] != j) {
				  scc_internal.bfs_component_sizes[j] = 0;
				} else {
				  scc_internal.bfs_component_sizes[Ci] -= scc_internal.bfs_component_sizes[j];
				}
			  }
			}
		  }
		}
		Ci = scc_internal.bfs_components[i];
		Cj = scc_internal.bfs_components[j];
	  }
	}


	free(action_stack);
	free(action_stack_components);

}



void stinger_scc_initialize_internals(struct stinger * S, int64_t nv, stinger_scc_internal* scc_internal, int64_t parentsPerVertex){
	scc_internal->bfsDataPtr = xmalloc(4*nv*sizeof(uint64_t));
	scc_internal->queue = &(scc_internal->bfsDataPtr[0]);
	scc_internal->level = &(scc_internal->bfsDataPtr[nv]);
	scc_internal->found = &(scc_internal->bfsDataPtr[2*nv]);
	scc_internal->same_level_queue = &(scc_internal->bfsDataPtr[3*nv]);
	scc_internal->parentsPerVertex = parentsPerVertex;

	scc_internal->parentsDataPtr	= xmalloc((parentsPerVertex + 3) * nv * sizeof(uint64_t));	
	scc_internal->parentArray		= &(scc_internal->parentsDataPtr[0]);
	scc_internal->parentCounter	    = &(scc_internal->parentsDataPtr[(parentsPerVertex) * nv]);
	scc_internal->bfs_components	= &(scc_internal->parentsDataPtr[(parentsPerVertex + 1) * nv]);
	scc_internal->bfs_component_sizes = &(scc_internal->parentsDataPtr[(parentsPerVertex + 2) * nv]);

	scc_internal->nv = nv;

	OMP("omp parallel for")
	for(int i = 0; i < nv; i++) {
		scc_internal->bfs_components[i] = i;
		scc_internal->bfs_component_sizes[i] = 0;
		scc_internal->level[i] = INFINITY_MY;
		scc_internal->parentCounter[i] = 0;
	}

	scc_internal->initCCCount = 0;
	for(uint64_t i = 0; i < nv; i++) {
		if(scc_internal->level[i] == INFINITY_MY) {
		 	scc_internal->bfs_component_sizes[i] =  bfs_build_component(S, i, scc_internal->queue, scc_internal->level, 
		 		scc_internal->parentArray, scc_internal->parentsPerVertex, scc_internal->parentCounter, scc_internal->bfs_components);
		 	scc_internal->initCCCount++;
		}
	}
}

void stinger_scc_release_internals(stinger_scc_internal* scc_internal){
  free(scc_internal->bfsDataPtr); 
  free(scc_internal->parentsDataPtr);
}



// The following is a demo main on how to run the streaming connected components code.
int scc_main_demo(struct stinger * S, int64_t nv, int64_t *batch,int64_t batch_size){

	stinger_scc_internal scc_internal;
	stinger_scc_initialize_internals(S,nv,&scc_internal,4);

	stinger_connected_components_stats stats;

	while(1){
		stinger_scc_reset_stats(&stats);

		const int64_t batchSizeExample=100;
		// Of course use meaningful batches...
		stinger_edge_update insertion[batchSizeExample],deletion[batchSizeExample];

		stinger_scc_insertion(S,nv,scc_internal,&stats,insertion,batchSizeExample);
		stinger_scc_deletion(S,nv,scc_internal,&stats,deletion,batchSizeExample);

		stinger_scc_print_insert_stats(&stats);
		stinger_scc_print_delete_stats(&stats);
		const int64_t* actual_components = stinger_scc_get_components(scc_internal);

		uint64_t actual_num_components=0;
		  	for(uint64_t v = 0; v < nv; v++) {
			    if (v==actual_components[v])
			      actual_num_components++;
		  	}  

		// break; // For one batch
		// continue // For numerous batches
	}

	stinger_scc_release_internals(&scc_internal);

}


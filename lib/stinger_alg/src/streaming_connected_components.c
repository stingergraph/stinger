
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

#define CC_STATS 1
#define CSV 0

#if CC_STATS
	static uint64_t bfs_deletes_in_tree = 0;
	static uint64_t bfs_inserts_in_tree_as_parents = 0;
	static uint64_t bfs_inserts_in_tree_as_neighbors = 0;
	static uint64_t bfs_inserts_in_tree_as_replacement = 0;
	static uint64_t bfs_inserts_bridged = 0;
	static uint64_t bfs_real_deletes = 0;
	static uint64_t bfs_real_inserts = 0;
	static uint64_t bfs_total_deletes = 0;
	static uint64_t bfs_total_inserts = 0;
	static uint64_t bfs_unsafe_deletes = 0;
	#if CSV
	#define CC_STAT_START(X) printf("\n%ld,", X)
	#define CC_STAT_INT64(X,Y) printf("\"%s\",%ld,",X,Y)
	#define CC_STAT_DOUBLE(X,Y) printf("\"%s\",%lf,",X,Y)
	static char filename[300];
	#else
	#define CC_STAT_START(X) //PRINT_STAT_INT64("stats_start", X)
	#define CC_STAT_INT64(X,Y) //PRINT_STAT_INT64(X,Y)
	#define CC_STAT_DOUBLE(X,Y) //PRINT_STAT_DOUBLE(X,Y)
	#endif	
	#define CC_STAT(X) X
#else
	#define CC_STAT_START(X)
	#define CC_STAT_INT64(X,Y) 
	#define CC_STAT_DOUBLE(X,Y) 
	#define CC_STAT(X) 
#endif


static int64_t nv, ne, naction;
static int64_t * restrict off;
static int64_t * restrict from;
static int64_t * restrict ind;
static int64_t * restrict weight;
static int64_t * restrict action;

/* handles for I/O memory */
static int64_t * restrict graphmem;
static int64_t * restrict actionmem;

// static char * initial_graph_name = INITIAL_GRAPH_NAME_DEFAULT;
// static char * action_stream_name = ACTION_STREAM_NAME_DEFAULT;
// static long batch_size = BATCH_SIZE_DEFAULT;
// static long nbatch = NBATCH_DEFAULT;

static char * initial_graph_name;
static char * action_stream_name;
static long batch_size;
static long nbatch;


static struct stinger * S;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * inline function definitions
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

inline void update_tree_for_delete_directed (int64_t * parentArray, int64_t * parentCounter, 
	int64_t * level, int64_t parentsPerVertex, int64_t i, int64_t j);

inline int64_t update_tree_for_insert_directed (int64_t * parentArray, int64_t * parentCounter, 
	int64_t * level,int64_t * component,int64_t parentsPerVertex, int64_t i, int64_t j);

inline int64_t is_delete_unsafe (int64_t * parentArray, int64_t * parentCounter, 
	int64_t * level,int64_t parentsPerVertex, int64_t i); 

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

  CC_STAT(uint64_t depth = 0);

  /* while queue is not empty */
  	while(qStart != qEnd) {
		uint64_t old_qEnd = qEnd;

		CC_STAT(depth++);

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

  CC_STAT(char component_name[256]);
  CC_STAT(sprintf(component_name, "component_depth[%ld]", currRoot));
  CC_STAT_INT64(component_name, depth);

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

  CC_STAT(uint64_t depth = 0);

  /* while queue is not empty */
  while(qStart != qEnd) {
	uint64_t old_qEnd = qEnd;

	CC_STAT(depth++);

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

  CC_STAT(char component_name[256]);
  CC_STAT(sprintf(component_name, "connection_depth[%ld]", component_id));
  CC_STAT_INT64(component_name, depth);

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
	int64_t * level,int64_t parentsPerVertex, int64_t i, int64_t j) {
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
	  CC_STAT(stinger_int64_fetch_add(&bfs_deletes_in_tree, 1));
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
	int64_t * level,int64_t * component,int64_t parentsPerVertex, int64_t i, int64_t j) {
  if(component[i] == component[j]) {
	int64_t local_parent_counter = readfe((uint64_t *)(parentCounter + j));
	if(LEVEL_IS_NEG(j)) {
	  if(LEVEL_IS_POS(i) && level[i] < ~level[j]) {
		if(local_parent_counter < parentsPerVertex) {
		  parentArray[j*parentsPerVertex + local_parent_counter] = i;
		  local_parent_counter++;
		  CC_STAT(stinger_int64_fetch_add(&bfs_inserts_in_tree_as_parents, 1));
		} else {
		  parentArray[j*parentsPerVertex] = i;
		}
		level[j] = ~level[j];
	  } 
	} else if(LEVEL_IS_POS(i)) {
	  if(local_parent_counter < parentsPerVertex) {
		if(level[i] < level[j]) {
		  parentArray[j*parentsPerVertex + local_parent_counter] = i;
		  local_parent_counter++;
		  CC_STAT(stinger_int64_fetch_add(&bfs_inserts_in_tree_as_parents, 1));
		} else if(level[i] == level[j]) {
		  parentArray[j*parentsPerVertex + local_parent_counter] = ~i;
		  local_parent_counter++;
		  CC_STAT(stinger_int64_fetch_add(&bfs_inserts_in_tree_as_neighbors, 1));
		}
	  } else if(level[i] < level[j]) {
		/* search backward - more likely to have neighbors at end */
		for(int64_t p = parentsPerVertex - 1; p >= 0; p--) {
		  if(parentArray[j*parentsPerVertex + p] < 0) {
			parentArray[j*parentsPerVertex + p] = i;
			CC_STAT(stinger_int64_fetch_add(&bfs_inserts_in_tree_as_replacement, 1));
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
	int64_t * level,int64_t parentsPerVertex, int64_t i){
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
	  fprintf(stderr,"\n\t%s %ld You shouldn't see this run.", __func__, __LINE__);
	  level[i] = ~level[i];
	}
	/* if no safe neighbors this delete is dangerous */
	if(!i_neighbors) {
	  return 1;
	}
  }
  return 0;
}

int is_tree_wrong (int64_t * parentArray, int64_t * parentCounter, int64_t parentsPerVertex, 
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

  CC_STAT_INT64("bfs_component_stats_max", max);
  CC_STAT_INT64("bfs_component_stats_min", min);
  CC_STAT_DOUBLE("bfs_component_stats_avg", avg);
  CC_STAT_INT64("bfs_component_stats_num", num_components);

  for(int64_t i = 0; i < histogram_max; i++) {
	char histogram_string[256];
	sprintf(histogram_string, "bfs_componnet_histogra_%ld", i);
	CC_STAT_INT64(histogram_string, size_histogram[i]);
  }

  if(previous_num < num_components) {
	CC_STAT_INT64("bfs_component_created", num_components - previous_num);
  } else {
	CC_STAT_INT64("bfs_component_joined", previous_num - num_components);
  }

  return num_components;
}

// int main (const int argc, char *argv[])
int streaming_connected_components (const int argc, char *argv[])
{
  parse_args (argc, argv, &initial_graph_name, &action_stream_name, &batch_size, &nbatch);

  load_graph_and_action_stream (initial_graph_name, &nv, &ne, (int64_t**)&off,
	  (int64_t**)&ind, (int64_t**)&weight, (int64_t**)&graphmem,
	  action_stream_name, &naction, (int64_t**)&action, (int64_t**)&actionmem);

  /* Convert to STINGER */
  tic ();
  S = stinger_new ();
  stinger_set_initial_edges (S, nv, 0, off, ind, weight, NULL, NULL, 0);
  //PRINT_STAT_DOUBLE ("time_stinger", toc ());
  fflush(stdout);

  free(graphmem);


  uint64_t * components = xmalloc(nv * sizeof(uint64_t));
  tic();
  uint64_t num_components = parallel_shiloach_vishkin_components_of_type(S, components,0);
  double sv_time = toc();
  //PRINT_STAT_INT64("components before",  num_components); 
  //PRINT_STAT_DOUBLE("shiloach vishkin time", sv_time);
  fflush(stdout);

  int64_t * queue = xmalloc(4*nv*sizeof(uint64_t));
  int64_t * level = &(queue[nv]);
  int64_t * found = &(queue[2*nv]);
  int64_t * same_level_queue = &(queue[3*nv]);

  int64_t   parentsPerVertex = 4;
  int64_t * parentArray		= xmalloc((parentsPerVertex + 3) * nv * sizeof(uint64_t));
  int64_t * parentCounter	= &(parentArray[(parentsPerVertex) * nv]);
  int64_t * bfs_components	= &(parentArray[(parentsPerVertex + 1) * nv]);
  int64_t * bfs_component_sizes = &(parentArray[(parentsPerVertex + 2) * nv]);

  tic ();
  OMP("omp parallel for")
	for(int i = 0; i < nv; i++) {
	  bfs_components[i] = i;
	  bfs_component_sizes[i] = 0;
	  level[i] = INFINITY_MY;
	  parentCounter[i] = 0;
	}

  CC_STAT_START(-1L);
  uint64_t bfs_num_components = 0;
  for(uint64_t i = 0; i < nv; i++) {
	if(level[i] == INFINITY_MY) {
	  bfs_component_sizes[i] = 
		bfs_build_component (
			S, i, queue,level, parentArray, 
			parentsPerVertex, parentCounter, bfs_components);
	  bfs_num_components++;
	}
  }
  double time_bfs = toc ();

  /* Updates */
  int64_t ntrace = 0;

  int64_t * action_stack = malloc(sizeof(int64_t) * batch_size * 2 * 2);
  int64_t * action_stack_components = malloc(sizeof(int64_t) * batch_size * 2 * 2);
  int64_t delete_stack_top;
  int64_t insert_stack_top;

  for (int64_t actno = 0; actno < nbatch * batch_size; actno += batch_size) {
	tic();
	delete_stack_top = 0;
	insert_stack_top = batch_size * 2 * 2 - 2;

	const int64_t endact = (actno + batch_size > naction ? naction : actno + batch_size);
	int64_t *actionStream = &action[2*actno];
	int64_t numActions = endact - actno;

	stinger_connected_components_stats stats;
	stinger_scc_reset_stats(&stats);

#if 1
	/* parallel for all insertions */
	OMP("omp parallel for reduction(+:bfs_real_inserts, bfs_total_inserts, bfs_inserts_bridged)")
	for (int64_t k = 0; k < numActions; k++) {
		int64_t i = actionStream[2 * k];
		int64_t j = actionStream[2 * k + 1];

		/* if an insertion and not self-loop */
		if(i >= 0 && i != j) {
		  CC_STAT(bfs_total_inserts += 2);
		  /* if a new edge in stinger, update parents */
		  if(stinger_insert_edge(S, 0, i, j, 1, ntrace+1) == 1) {
			CC_STAT(bfs_real_inserts++);
			if(update_tree_for_insert_directed(parentArray, parentCounter, level, bfs_components, parentsPerVertex, i, j)) {
			  CC_STAT(bfs_inserts_bridged++);
			  /* if component ids are not the same       *
			   * tree update not handled, push onto queue*/
			  int64_t which = stinger_int64_fetch_add(&insert_stack_top, -2);
			  action_stack[which] = i;
			  action_stack[which+1] = j;
			}
		  }
		  /* same but for reverse edge j,i */
		  if(stinger_insert_edge(S, 0, j, i, 1, ntrace+1) == 1) {
			CC_STAT(bfs_real_inserts++);
			if(update_tree_for_insert_directed(parentArray, parentCounter, level, bfs_components, parentsPerVertex, j, i)) {
			  CC_STAT(bfs_inserts_bridged++);
			  int64_t which = stinger_int64_fetch_add(&insert_stack_top, -2);
			  action_stack[which] = j;
			  action_stack[which+1] = i;
			}
		  }
		}
	} 

	/* serial for-all insertions that joined components */
	for(int64_t k = batch_size * 2 * 2 - 2; k > insert_stack_top; k -= 2) {
	  int64_t i = action_stack[k]; 
	  int64_t j = action_stack[k+1];

	  int64_t Ci = bfs_components[i];
	  int64_t Cj = bfs_components[j];

	  if(Ci == Cj)
		continue;

	  int64_t Ci_size = bfs_component_sizes[Ci];
	  int64_t Cj_size = bfs_component_sizes[Cj];

	  if(Ci_size > Cj_size) {
		SWAP_UINT64(i,j)
		SWAP_UINT64(Ci, Cj)
		SWAP_UINT64(Ci_size, Cj_size)
	  }

	  parentArray[i*parentsPerVertex] = j;
	  parentCounter[i] = 1;
	  /* handle singleton */
	  if(Ci_size == 1) {
		bfs_component_sizes[Ci] = 0;
		bfs_component_sizes[Cj]++;
		bfs_components[i] = Cj;
	  } else {
		bfs_component_sizes[Cj] += bfs_build_new_component(S, i, Cj, (level[j] >= 0 ? level[j] : ~level[j])+1, queue, level, parentArray, parentsPerVertex, parentCounter, bfs_components);
		bfs_component_sizes[Ci] = 0;
	  }
	}

	// CC_STAT(bfs_num_components = bfs_component_stats(nv, bfs_component_sizes, bfs_num_components));
	stinger_scc_print_insert_stats(&stats);
#endif

#if 1
	/* Parallel forall deletions */
	OMP("omp parallel for reduction(+:bfs_total_deletes, bfs_real_deletes)")
	for (int64_t k = 0; k < numActions; k++) {
		int64_t i = actionStream[2 * k];
		int64_t j = actionStream[2 * k + 1];

		/* is a delete and not a self-loop */
		if(i < 0 && i != j) {
		  CC_STAT(bfs_total_deletes += 2);
		  /* for contenience */
		  i = ~i;
		  j = ~j;

		  /* if a real edge in stinger, update parents, push onto queue to check safety later */
		  if(stinger_remove_edge(S, 0, i, j) == 1) {
			CC_STAT(bfs_real_deletes++);
			uint64_t which = stinger_int64_fetch_add(&delete_stack_top, 2);
			action_stack[which] = i;
			action_stack[which+1] = j;
			action_stack_components[which] = bfs_components[i];
			action_stack_components[which+1] = bfs_components[j];
			update_tree_for_delete_directed(parentArray, parentCounter, level, parentsPerVertex, i, j);
		  }

		  /* if a real edge in stinger, update parents, push onto queue to check safety later */
		  if(stinger_remove_edge(S, 0, j, i) == 1) {
			CC_STAT(bfs_real_deletes++);
			uint64_t which = stinger_int64_fetch_add(&delete_stack_top, 2);
			action_stack[which] = j;
			action_stack[which+1] = i;
			action_stack_components[which] = bfs_components[j];
			action_stack_components[which+1] = bfs_components[i];
			update_tree_for_delete_directed(parentArray, parentCounter, level, parentsPerVertex, j, i);
		  }
		} 
	  }

	/* parallel safety check */
	OMP("omp parallel for reduction(+:bfs_unsafe_deletes)")
	  for(uint64_t k = 0; k < delete_stack_top; k += 2) {
		int64_t i = action_stack[k];
		int64_t j = action_stack[k+1];
		if(!(is_delete_unsafe(parentArray, parentCounter, level, parentsPerVertex, i))) {
		  action_stack[k] = -1;
		  action_stack[k+1] = -1;
		}
		CC_STAT(else bfs_unsafe_deletes += 2);
	}

	/* explicitly not parallel for all unsafe deletes */
	for(uint64_t k = 0; k < delete_stack_top; k += 2) {
	  int64_t i = action_stack[k];
	  int64_t j = action_stack[k+1];
	  int64_t Ci_prev = action_stack_components[k];
	  int64_t Cj_prev = action_stack_components[k+1];
	  if(i != -1)  {
		int64_t Ci = bfs_components[i];
		int64_t Cj = bfs_components[j];

		if(Ci == Cj && Ci == Ci_prev) {
		  if(Ci != i && stinger_outdegree(S, i) == 0) {
			bfs_component_sizes[Ci]--;
			bfs_components[i] = i;
			bfs_component_sizes[i] = 1;
		  } else if(Cj != j && stinger_outdegree(S, j) == 0) {
			bfs_component_sizes[Cj]--;
			bfs_components[j] = j;
			bfs_component_sizes[j] = 1;
		  } else {
			int64_t level_i = level[i];
			int64_t level_j = level[j];

			if(level_i < 0)
			  level_i = ~level_i;
			if(level_j < 0)
			  level_j = ~level_j;

			if(level_i > level_j) {
			  if(is_delete_unsafe(parentArray, parentCounter, level, parentsPerVertex, i)) {
				parentCounter[i] = 0;
				bfs_component_sizes[i] = bfs_rebuild_component(S, i, i, (level[i] >= 0 ? level[i] : ~level[i]), queue, level, parentArray, parentsPerVertex, parentCounter, bfs_components, same_level_queue);
				if(bfs_components[i] != i) {
				  bfs_component_sizes[i] = 0;
				} else {
				  bfs_component_sizes[Cj] -= bfs_component_sizes[i];
				}
			  }
			} else {
			  if(is_delete_unsafe(parentArray, parentCounter, level, parentsPerVertex, j)) {
				parentCounter[j] = 0;
				bfs_component_sizes[j] = bfs_rebuild_component(S, j, j, (level[j] >= 0 ? level[j] : ~level[j]), queue, level, parentArray, parentsPerVertex, parentCounter, bfs_components, same_level_queue);
				if(bfs_components[j] != j) {
				  bfs_component_sizes[j] = 0;
				} else {
				  bfs_component_sizes[Ci] -= bfs_component_sizes[j];
				}
			  }
			}
		  }
		}
		Ci = bfs_components[i];
		Cj = bfs_components[j];
	  }
	}

	// CC_STAT(bfs_num_components = bfs_component_stats(nv, bfs_component_sizes, bfs_num_components));
	stinger_scc_print_delete_stats(&stats);
#endif

	num_components = parallel_shiloach_vishkin_components_of_type(S, components,0);
	//PRINT_STAT_INT64("shiloach vishkin components", num_components); fflush(stdout);
	{
	  uint64_t cmp = 0;
	  OMP("omp parallel for reduction(+:cmp)")
		for(uint64_t p = 0; p < nv; p++) {
		  uint64_t Cp_shiloach = components[p];
		  if(bfs_components[p] != bfs_components[Cp_shiloach])
			cmp++;
		}
	  //PRINT_STAT_INT64("bfs_components_wrong", cmp);
	}
	////PRINT_STAT_INT64("full_tree_check", is_full_tree_wrong(S,nv,parentArray,parentCounter,level,parentsPerVertex));

	ntrace++;
  } /* End of batch */

  // num_components = parallel_shiloach_vishkin_components_of_type(S, components,0);


  free(queue); 
  free(parentArray);

  stinger_free_all (S);
  free (actionmem);
  // STATS_END();
}


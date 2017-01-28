#include "core_util.h"
#include "xmalloc.h"
#include "stinger_atomics.h"
#include "stinger.h"

void update_edge_data (struct stinger * S, struct stinger_eb *eb,
                  uint64_t index, int64_t neighbor, int64_t weight, int64_t ts, int64_t operation) {

  update_edge_data_and_direction (S, eb, index, neighbor, weight, ts, STINGER_EDGE_DIRECTION_OUT, operation);
}

 
/** @brief DEPRECATED Remove and insert edges incident on a common source vertex.
 *
 *  DEPRECATED
 *  For a given source vertex and edge type, removes a list of edges given in
 *  an array.  Then inserts a list of edges given in a second array with weight
 *  and a timestamp.
 *  Note: Do not call this function concurrently with the same source vertex,
 *  even for different edge types.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param nremove Number of edges to remove
 *  @param remove Edge adjacencies to remove
 *  @param ninsert Number of edges to insert
 *  @param insert Edge adjacencies to insert
 *  @param weight Edge adjacency weights to insert
 *  @param timestamp Timestamp for all edge insertions
 *  @return 0 on success; number of failures otherwise
 */
int
stinger_remove_and_insert_edges (struct stinger *G,
                                 int64_t type, int64_t from,
                                 int64_t nremove, int64_t * remove,
                                 int64_t ninsert, int64_t * insert,
                                 int64_t * weight, int64_t timestamp)
{
  /* Do *NOT* call this concurrently with the same from value, even
     with different types. */
  /*
    First pass:
    - Remove vertices.
    - Scan for insertion slots.
    - Scan for existing vertices being inserted.
    Second:
    - Insert into saved locations.
  */

  MAP_STING(G);

  int64_t nrem = 0;
  struct curs curs;
  struct stinger_eb *tmp;
  eb_index_t *prev_loc;
  struct stinger_eb **has_slot = NULL;
  int *kslot = NULL;
  int64_t nslot = 0, ninsert_remaining = ninsert;

  if (1 == ninsert) {
    if (insert[0] >= 0)
      nrem += stinger_insert_edge (G, type, from, insert[0],
                                  (weight ? weight[0] : 1), timestamp);
    ninsert = 0;
  }

  if (1 == nremove) {
    if (remove[0] >= 0)
      nrem = stinger_remove_edge (G, type, from, remove[0]);
    nremove = 0;
  }

  /* Sort for binary search */
  if (nremove) {
    qsort (remove, nremove, sizeof (*remove), i64_cmp);
    while (nremove && *remove < 0) {
      ++remove;
      --nremove;
    }
  }
  if (ninsert) {
    has_slot = xmalloc (ninsert * sizeof (*has_slot));
    kslot = xmalloc (ninsert * sizeof (*kslot));
    qsort (insert, ninsert, sizeof (*insert), i64_cmp);
    while (ninsert && *insert < 0) {
      ++insert;
      --ninsert;
    }
  }

  if (!nremove && !ninsert) {
    return nrem;
  }

  curs = etype_begin (G, from,type);
  prev_loc = curs.loc;

  struct stinger_eb * ebpool_priv = ebpool->ebpool;

  for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + tmp->next) {
    if(tmp->etype == type) {
      size_t k, endk;
      /*size_t found_k = STINGER_EDGEBLOCKSIZE;*/
      /*size_t found_slot = STINGER_EDGEBLOCKSIZE;*/
      prev_loc = &tmp->next;
      endk = tmp->high;
      ninsert_remaining = ninsert;

	for (k = 0; k < endk; ++k) {
	  const int64_t w = tmp->edges[k].neighbor;
	  int64_t off;

	  if (w >= 0) {
	    /* Removal first.  If the vertex is removed,
	       save its slot for reuse. */
	    if (nremove) {
	      off = find_in_sorted (w, nremove, remove);
	      if (off >= 0) {
		int64_t which = stinger_int64_fetch_add (&nslot, 1);
		if (which < ninsert_remaining) {    /* Can be racy. */
		  has_slot[which] = tmp;
		  kslot[which] = k;
		}
		update_edge_data(G, tmp, k, ~w, 0, timestamp, EDGE_WEIGHT_SET);
		stinger_int64_fetch_add (&nrem, 1);
	      }
	    }
	    if (ninsert) {
	      /* Check if this is to be inserted. */
	      off = find_in_sorted (w, ninsert, insert);
	      while (off >= 0) {
		stinger_int64_fetch_add (&ninsert_remaining, -1);
		insert[off] = ~insert[off];
		update_edge_data(G, tmp, k, w, (weight ? weight[off] : 1), timestamp, EDGE_WEIGHT_SET);
		qsort (insert, ninsert, sizeof (*insert), i64_cmp);  /* Must maintain sorted order here. */
		off = find_in_sorted (w, ninsert, insert);   /* Gotta check if there's another one. */
	      }
	    }
	  } else if (ninsert_remaining) {
	    /* Empty slot.  Save. */
	    int64_t which = stinger_int64_fetch_add (&nslot, 1);
	    if (which < ninsert_remaining) {        /* Can be racy. */
	      has_slot[which] = tmp;
	      kslot[which] = k;
	    }
	  }
	}

	if (nslot < ninsert_remaining) {
	/* Gather any trailing slots. */
	
	  for (; endk < STINGER_EDGEBLOCKSIZE; ++endk) {
	    int64_t which = stinger_int64_fetch_add (&nslot, 1);
	    if (which < ninsert_remaining) {        /* Can be racy. */
	      has_slot[which] = tmp;
	      kslot[which] = endk;
	    }
	  }
      }
    }
  }

  /* At this point, know how many need inserted & how many slots
     already are available. */

  if (ninsert_remaining > nslot) {
    /* Need more edge blocks. */
    int64_t neb =
      (ninsert_remaining + STINGER_EDGEBLOCKSIZE - 1) / STINGER_EDGEBLOCKSIZE;
    eb_index_t *ebs;
    ebs = xmalloc (neb * sizeof (*ebs));

    new_ebs (G, ebs, neb, type, from);
    for (int64_t kb = 0; kb < neb - 1; ++kb)
      ebpool_priv[ebs[kb]].next = kb + 1;
    ebpool_priv[ebs[neb - 1]].next = 0;
    *prev_loc = ebs[0];
    push_ebs (G, neb, ebs);

    
      for (int64_t kb = 0; kb < neb; ++kb) {
        struct stinger_eb *eb = ebpool_priv + ebs[kb];
          for (int64_t k = 0;
               nslot < ninsert_remaining
                 && k <
                 STINGER_EDGEBLOCKSIZE; ++k) {
            int64_t which = stinger_int64_fetch_add (&nslot, 1);
            if (which < ninsert_remaining) {        /* Can be racy. */
              has_slot[which] = eb;
              kslot[which] = k;
            }
          }
      }

    free (ebs);
  }

  if (ninsert_remaining) {
    nslot = 0;
    for (int64_t k = 0; k < ninsert; ++k) {
      const int64_t w = insert[k];
      if (w >= 0) {
        int64_t ks;
        struct stinger_eb *restrict eb;
        int64_t which = stinger_int64_fetch_add (&nslot, 1);

        assert (which < ninsert_remaining);
        ks = kslot[which];
        eb = has_slot[which];

        assert (ks < STINGER_EDGEBLOCKSIZE);
        assert (ks >= 0);
        assert (eb);
        /* Breaking atomicity => assert may break. */
        update_edge_data (G, eb, ks, w,
                          (weight ? weight[k] : 1), timestamp, EDGE_WEIGHT_SET);
      }
    }
  }

  free (kslot);
  free (has_slot);
  return (nrem + ninsert_remaining) > 0;
}


/** @brief DEPRECATED Process edge removals and insertions as a batch.
 *
 *  DEPRECATED
 *  Takes its input from stinger_sort_actions().  Takes a sorted batch of edge
 *  insertions and removals and processes them in parallel in the data structure.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param timestamp The current timestamp for this batch
 *  @param n Number of incident vertices in the batch
 *  @param insoff For each incident vertex, the offset into the actions array of insertions
 *  @param deloff For each incident vertex, the offset into the actions array of deletions
 *  @param act The sorted actions array
 *  @return The number of incident vertices
 */
int64_t
stinger_remove_and_insert_batch (struct stinger * G, int64_t type,
                                 int64_t timestamp, int64_t n,
                                 int64_t * insoff, int64_t * deloff,
                                 int64_t * act)
{
  /* Separate each vertex's batch into deletions and insertions */
  OMP ("omp parallel for")
  
  for (int k = 0; k < n; k++) {
    const int64_t i = act[2 * deloff[k]];
    const int64_t nrem = insoff[k] - deloff[k];
    const int64_t nins = deloff[k + 1] - insoff[k];
    int64_t *restrict torem, *restrict toins;

    torem = (nrem + nins ? xmalloc ((nrem + nins) * sizeof (*torem)) : NULL);
    toins = (torem ? &torem[nrem] : NULL);

    
    for (int k2 = 0; k2 < nrem; ++k2) {
      torem[k2] = act[2 * (deloff[k] + k2) + 1];
      assert (act[2 * (deloff[k] + k2)] == i);
    }

    
    for (int k2 = 0; k2 < nins; k2++) {
      toins[k2] = act[2 * (insoff[k] + k2) + 1];
      assert (act[2 * (insoff[k] + k2)] == i);
    }

/* Do the update in the data structure */
    stinger_remove_and_insert_edges (G, type, i, nrem, torem, nins, toins,
                                       NULL, timestamp);
      if (torem != NULL)
	free (torem);
    }

  return n;
}

/** @brief DEPRECATED Copy typed adjacencies of a vertex into a buffer
 *
 *  DEPRECATED
 *  For a given edge type, adjacencies of the specified vertex are copied into
 *  the user-provided buffer up to the length of the buffer.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param v Source vertex ID
 *  @param outlen Number of adjacencies copied
 *  @param out Buffer to hold adjacencies
 *  @param max_outlen Length of out[] and recent[]
 *  @return Void
 */
void
stinger_gather_typed_successors_serial (struct stinger *G, int64_t type,
                                        int64_t v, size_t * outlen,
                                        int64_t * out, size_t max_outlen)
{
  size_t kout = 0;

  assert (G);

  if (v < 0) {
    *outlen = 0;
    return;
  }

  STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(G, v, type) {
    if(kout < max_outlen)
      out[kout++] = STINGER_EDGE_DEST;
  } STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END();

  *outlen = kout;               /* May be longer than max_outlen. */
}

#if defined(STINGER_FORCE_OLD_MAP)
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * OLD STINGER_PHYSMAP.C
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "stinger-atomics.h"

#define MARKERINT INT64_MAX
uint64_t MARKER_;
void * MARKER = (void *)&MARKER_;

#define CHILDREN_COUNT 256

/* struct defs */

typedef struct tree_node {
  struct tree_node * children[CHILDREN_COUNT];
  struct tree_node * parent;
  uint8_t isEndpoint;
  uint64_t vertexID;
  uint64_t depth;
  char value;
} tree_node_t;

struct stinger_physmap {
  tree_node_t keyTree[MAX_NODES];
  uint64_t keyTreeTop;
  tree_node_t * vtxStack[MAX_VTXID];
  uint64_t vtxStackTop;
};

/* internal functions protos */

tree_node_t *
allocateTreeNode (stinger_physmap_t * map, tree_node_t * parent, uint64_t depth, char value);

int
insertIntoTree(stinger_physmap_t * map, tree_node_t ** node, char * string, uint64_t length);

/* function defs */

/** @brief Allocate and initialize a new physical mapper.
 *
 *  The user is responsible for freeing via stinger_physmap_delete().
 *
 *  @return A new physical mapper.
 */
stinger_physmap_t * 
stinger_physmap_create() {
  stinger_physmap_t * out = malloc(sizeof(stinger_physmap_t));
  if(out) {
    out->vtxStackTop = 0;
    out->keyTreeTop = 0;
    allocateTreeNode(out, NULL, 0, '\0');
  }
  return out;
}

/** @brief Free a physical mapper.
 *
 *  @param map The physical mapper to be freed.
 */
void
stinger_physmap_delete(stinger_physmap_t * map) {
  free(map);
}

tree_node_t *
allocateTreeNode (stinger_physmap_t * map, tree_node_t * parent, uint64_t depth, char value) {
  uint64_t myNode = stinger_int64_fetch_add(&(map->keyTreeTop),1);
  if(map->keyTreeTop >= MAX_NODES) {
    fprintf(stderr, "PHYSMAP: ERROR Out of treenodes\n");
    return NULL;
  }
  memset(map->keyTree + myNode, 0, sizeof(tree_node_t));
  map->keyTree[myNode].parent = parent;
  map->keyTree[myNode].depth = depth;
  map->keyTree[myNode].value = value;
  return map->keyTree + myNode;
}

int
insertIntoTree(stinger_physmap_t * map, tree_node_t ** node, char * string, uint64_t length) {
  if(length == 0) {
    if((*node)->isEndpoint)
      return 1;
    (*node)->isEndpoint = 1;
    if(!stinger_int64_cas(&((*node)->vertexID), 0, MARKERINT)) {
      return 0;
    } else {
      return 2;
    }
  } else {
    if(!(*node)->children[string[0]]) {
      if(!stinger_ptr_cas((void **)&((*node)->children[string[0]]), NULL, MARKER)) {
	(*node)->children[string[0]] = allocateTreeNode(map, *node, (*node)->depth+1, string[0]);
      }
    }
    if(!(*node)->children[string[0]]) {
      return -1;
    }
    while((*node)->children[string[0]] == MARKER) ;
    (*node) = (*node)->children[string[0]];
    return insertIntoTree(map, node, ++string, --length);
  }
}

/** @brief Create a new mapping from a binary data string to a vertex ID.
 *
 *  This function will uniquely map an arbitrary binary string or character
 *  string to a vertex ID in the space of 0 to NV where NV is the number
 *  of unique strings that have been mapped thus far (in other words the vertex
 *  ID space is compact).  It will return -1 on error or if the mapping already exists.
 *  It is safe to call this function in parallel with any other physical mapper function.
 *  To determine if a -1 result is from an error, call stinger_physmap_get_mapping() 
 *  on the same string.  If it also returns -1, then an error has occurred.
 *
 *  @param map The physical mapper.
 *  @param string The binary or character data string.
 *  @param length The length of the string.
 *  @return A unique vertex ID or -1 if the mapping exists or an error occurs.
 */
uint64_t
stinger_physmap_create_mapping (stinger_physmap_t * map, char * string, uint64_t length) {
  if(map->vtxStackTop == MAX_VTXID) {
    fprintf(stderr, "PHYSMAP: ERROR Out of vertices\n");
    return -1;
  }
  uint64_t vertexID;
  tree_node_t * node = map->keyTree;
  int result = insertIntoTree(map, &node, string, length);
  switch(result) {
    case 0:
      vertexID = stinger_int64_fetch_add(&(map->vtxStackTop), 1);
      node->vertexID = vertexID;
      map->vtxStack[vertexID] = node;
      break;
    case 2:
      while(node->vertexID == MARKERINT) ;
      vertexID = node->vertexID;
    default:
      return -1;
  }
  return vertexID;
}

/** @brief Lookup a mapping from a binary data string to a vertex ID.
 *
 *  This function will lookup and return a previously created mapping.  It will return -1 
 *  if no mapping exists.
 *  It is safe to call this function in parallel with any other physical mapper function.
 *
 *  @param map The physical mapper.
 *  @param string The binary or character data string.
 *  @param length The length of the string.
 *  @return A unique vertex ID or -1 if the mapping does not exist.
 */
uint64_t
stinger_physmap_get_mapping (stinger_physmap_t * map, char * string, uint64_t length) {
  tree_node_t * cur = map->keyTree;
  while(length > 0 && cur) {
    cur = cur->children[string[0]];
    string++;
    length--;
  }
  if(cur && cur->isEndpoint) {
    return cur->vertexID;
  } else {
    return -1;
  }
}

/** @brief Lookup the string mapped to a particular vertex ID.
 *
 *  This function will lookup and return a previously created mapping.  It will return -1 
 *  no mapping exists or a reallocation of the output buffer fails.  If the output buffer
 *  is not long enough, this function will reallocate the buffer and update the output buffer
 *  length.
 *  It is safe to call this function in parallel with any other physical mapper function.
 *
 *  @param map The physical mapper.
 *  @param outbuffer A buffer to store the output string.
 *  @param outbufferlength The length of the buffer.
 *  @param vertexID The vertex ID to reverse lookup.
 *  @return 0 on success, -1 on failure.
 */
int
stinger_physmap_get_key (stinger_physmap_t * map, char ** outbuffer, uint64_t * outbufferlength, uint64_t vertexID) {
  if(vertexID >= map->vtxStackTop || map->vtxStack[vertexID] == NULL || (!map->vtxStack[vertexID]->isEndpoint)) 
    return -1;
  tree_node_t * node = map->vtxStack[vertexID];
  if(node->depth+1 > (*outbufferlength)) {
    char * tmpbuffer = realloc(*outbuffer, sizeof(char) * (node->depth+1));
    if(tmpbuffer) {
      (*outbuffer) = tmpbuffer;
      (*outbufferlength) = node->depth+1;
    } else {
      return -1;
    }
  }
  (*outbuffer)[node->depth] = '\0';
  while(node->parent) {
    (*outbuffer)[node->depth-1] = node->value;
    node = node->parent;
  }
  return 0;
}

uint64_t
stinger_physmap_remove_mapping (stinger_physmap_t * map, uint64_t vertexID) {
  printf("***TODO***\n");
}

/* Independent test-driver code */
#if PHYSMAP_TEST

#include <omp.h>
#include <sys/time.h>


struct timeval tv;

double firsttic = 0;
double lasttic = 0;

void tic_reset() {
	gettimeofday(&tv, NULL);
	firsttic = (double)tv.tv_sec + 1.0e-6 * (double)tv.tv_usec;
	lasttic = firsttic;
}
double tic_total() {
	gettimeofday(&tv, NULL);
	lasttic = (double)tv.tv_sec + 1.0e-6 * (double)tv.tv_usec;
	return lasttic - firsttic;
}
double tic_sincelast() {
	gettimeofday(&tv, NULL);
	double rtnval = ((double)tv.tv_sec + 1.0e-6 * (double)tv.tv_usec) - lasttic;
	lasttic = (double)tv.tv_sec + 1.0e-6 * (double)tv.tv_usec;
	return rtnval;
}

/*
 * Parallel test driver code
 */

int main(int argc, char *argv[]) {
  stinger_physmap_t * map = stinger_physmap_create();
  if(!map) {
    printf("ALLOC FAILED");
    return 0;
  }

  if(argc < 2) {
    return -1;
  }
  int threads = atoi(argv[1]);
  omp_set_num_threads(threads);
  uint64_t lines_in_file = 0;
  char ** strings;
  uint64_t * lengths;
  float insertion, lookup, reverselookup;
#pragma omp parallel
  {
#pragma omp master
    {
      printf("%d,", omp_get_num_threads());
      FILE * fp = fopen(argv[2], "r");
      char * string = malloc(100*sizeof(char));;
      uint64_t read = 0;
      int bytes = 100;
      while((read = getline(&string, &bytes, fp)) != EOF ) {
	lines_in_file++;
      }
      free(string);
      fclose(fp);
      fp = fopen(argv[2], "r");
      strings = malloc(lines_in_file * sizeof(char *));
      lengths = malloc(lines_in_file * sizeof(uint64_t));
      for(uint64_t i = 0; i < lines_in_file; ++i) {
	string = malloc(100*sizeof(char));;
	read = getline(&string, &bytes, fp);
	strings[i] = string;
	lengths[i] = read - 2;
      }
      printf("%d,",lines_in_file);
    }
  }

  tic_reset();
#pragma omp parallel for
  for(uint64_t i = 0; i < lines_in_file; ++i) {
    uint64_t mapping = stinger_physmap_create_mapping(map, strings[i ], lengths[i ]);
  }
  insertion = tic_sincelast();

#pragma omp parallel for
  for(uint64_t i = 0; i < lines_in_file; ++i) {
    uint64_t mapping = stinger_physmap_get_mapping(map, strings[i ], lengths[i ]);
    if(mapping == -1)
      printf("lu %s %lu %lu\n", strings[i ], lengths[i ], mapping);
  }
  lookup = tic_sincelast();

#pragma omp parallel
{
  char * string2 = malloc(sizeof(char) * 100);
  tic_reset();
#pragma omp for
  for(uint64_t i = 0; i < map->vtxStackTop; ++i) {
    uint64_t slen2 = 100;
    if(stinger_physmap_get_key(map, &string2, &slen2, i ))
      printf("rlu %s %lu\n", string2, slen2);
  }
}
  reverselookup = tic_sincelast();

  printf("%f,%f,%f,", insertion, lookup, reverselookup);
  printf("%f,%f,%f,\n", lines_in_file/insertion, lines_in_file/lookup, lines_in_file/reverselookup);
  stinger_physmap_delete(map);
}

#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * END STINGER_PHYSMAP.C
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif

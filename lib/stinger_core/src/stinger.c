/* -*- mode: C; mode: folding; fill-column: 70; -*- */
#include <dirent.h>
#include <limits.h>
#include <errno.h>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include "stinger.h"
#include "stinger_error.h"
#include "stinger_atomics.h"
#include "core_util.h"
#include "xmalloc.h"
#include "x86_full_empty.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * ACCESS INTERNAL "CLASSES"
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
inline stinger_vertices_t *
stinger_vertices_get(const stinger_t * S) {
  MAP_STING(S);
  return vertices;
}

inline stinger_physmap_t *
stinger_physmap_get(const stinger_t * S) {
  MAP_STING(S);
  return physmap;
}

inline stinger_names_t *
stinger_vtype_names_get(const stinger_t * S) {
  MAP_STING(S);
  return vtype_names;
}

inline stinger_names_t *
stinger_etype_names_get(const stinger_t * S) {
  MAP_STING(S);
  return etype_names;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * EXTERNAL INTERFACE FOR VERTICES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* IN DEGREE */

inline vdegree_t
stinger_indegree_get(const stinger_t * S, vindex_t v) {
  return stinger_vertex_indegree_get(stinger_vertices_get(S), v);
}

inline vdegree_t
stinger_indegree_set(const stinger_t * S, vindex_t v, vdegree_t d) {
  return stinger_vertex_indegree_set(stinger_vertices_get(S), v, d);
}

inline vdegree_t
stinger_indegree_increment(const stinger_t * S, vindex_t v, vdegree_t d) {
  return stinger_vertex_indegree_increment(stinger_vertices_get(S), v, d);
}

inline vdegree_t
stinger_indegree_increment_atomic(const stinger_t * S, vindex_t v, vdegree_t d) {
  return stinger_vertex_indegree_increment_atomic(stinger_vertices_get(S), v, d);
}

/* OUT DEGREE */

/* inline vdegree_t */
/* stinger_outdegree_get(const stinger_t * S, vindex_t v) { */
/*   return stinger_vertex_outdegree_get(stinger_vertices_get(S), v); */
/* } */

inline vdegree_t
stinger_outdegree_set(const stinger_t * S, vindex_t v, vdegree_t d) {
  return stinger_vertex_outdegree_set(stinger_vertices_get(S), v, d);
}

inline vdegree_t
stinger_outdegree_increment(const stinger_t * S, vindex_t v, vdegree_t d) {
  return stinger_vertex_outdegree_increment(stinger_vertices_get(S), v, d);
}

inline vdegree_t
stinger_outdegree_increment_atomic(const stinger_t * S, vindex_t v, vdegree_t d) {
  return stinger_vertex_outdegree_increment_atomic(stinger_vertices_get(S), v, d);
}

/* TYPE */

vtype_t
stinger_vtype_get(const stinger_t * S, vindex_t v) {
  MAP_STING(S);
  return stinger_vertex_type_get(vertices, v);
}

vtype_t
stinger_vtype_set(const stinger_t * S, vindex_t v, vtype_t type) {
  MAP_STING(S);
  return stinger_vertex_type_set(vertices, v, type);
}

/* WEIGHT */

vweight_t
stinger_vweight_get(const stinger_t * S, vindex_t v) {
  MAP_STING(S);
  return stinger_vertex_weight_get(vertices, v);
}

vweight_t
stinger_vweight_set(const stinger_t * S, vindex_t v, vweight_t weight) {
  MAP_STING(S);
  return stinger_vertex_weight_set(vertices, v, weight);
}

vweight_t
stinger_vweight_increment(const stinger_t * S, vindex_t v, vweight_t weight) {
  MAP_STING(S);
  return stinger_vertex_weight_increment(vertices, v, weight);
}

vweight_t
stinger_vweight_increment_atomic(const stinger_t * S, vindex_t v, vweight_t weight) {
  MAP_STING(S);
  return stinger_vertex_weight_increment_atomic(vertices, v, weight);
}

/* ADJACENCY */
adjacency_t
stinger_adjacency_get(const stinger_t * S, vindex_t v) {
  MAP_STING(S);
  return stinger_vertex_edges_get(vertices, v);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * EXTERNAL INTERFACE FOR PHYSMAP
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int
stinger_mapping_create(const stinger_t * S, const char * byte_string, uint64_t length, int64_t * vtx_out) {
  return stinger_physmap_mapping_create(stinger_physmap_get(S), stinger_vertices_get(S), byte_string, length, vtx_out);
}

vindex_t
stinger_mapping_lookup(const stinger_t * S, const char * byte_string, uint64_t length) {
  return stinger_physmap_vtx_lookup(stinger_physmap_get(S), stinger_vertices_get(S), byte_string, length);
}

int
stinger_mapping_physid_get(const stinger_t * S, vindex_t vertexID, char ** outbuffer, uint64_t * outbufferlength) {
  return stinger_physmap_id_get(stinger_physmap_get(S), stinger_vertices_get(S), vertexID, outbuffer, outbufferlength);
}

int
stinger_mapping_physid_direct(const stinger_t * S, vindex_t vertexID, char ** out_ptr, uint64_t * out_len) {
  return stinger_physmap_id_direct(stinger_physmap_get(S), stinger_vertices_get(S), vertexID, out_ptr, out_len);
}

vindex_t
stinger_mapping_nv(const stinger_t * S) {
  return stinger_physmap_nv(stinger_physmap_get(S));
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * EXTERNAL INTERFACE FOR VTYPE NAMES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int
stinger_vtype_names_create_type(const stinger_t * S, const char * name, int64_t * out) {
  return stinger_names_create_type(stinger_vtype_names_get(S), name, out);
}

int64_t
stinger_vtype_names_lookup_type(const stinger_t * S, const char * name) {
  return stinger_names_lookup_type(stinger_vtype_names_get(S), name);
}

char *
stinger_vtype_names_lookup_name(const stinger_t * S, int64_t type) {
  return stinger_names_lookup_name(stinger_vtype_names_get(S), type);
}

int64_t
stinger_vtype_names_count(const stinger_t * S) {
  return stinger_names_count(stinger_vtype_names_get(S));
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * EXTERNAL INTERFACE FOR ETYPE NAMES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int
stinger_etype_names_create_type(stinger_t * S, const char * name, int64_t * out) {
  return stinger_names_create_type(stinger_etype_names_get(S), name, out);
}

int64_t
stinger_etype_names_lookup_type(const stinger_t * S, const char * name) {
  return stinger_names_lookup_type(stinger_etype_names_get(S), name);
}

char *
stinger_etype_names_lookup_name(const stinger_t * S, int64_t type) {
  return stinger_names_lookup_name(stinger_etype_names_get(S), type);
}

int64_t
stinger_etype_names_count(const stinger_t * S) {
  return stinger_names_count(stinger_etype_names_get(S));
}


/* {{{ Edge block pool */
/* TODO XXX Rework / possibly move EB POOL functions */



MTA ("mta expect parallel context") MTA ("mta inline")
static void
get_from_ebpool (const struct stinger * S, eb_index_t *out, size_t k)
{
  MAP_STING(S);
  eb_index_t ebt0;
  {
    ebt0 = stinger_int64_fetch_add (&(ebpool->ebpool_tail), k);
    if (ebt0 + k >= (S->max_neblocks)) {
      LOG_F("STINGER has run out of internal storage space.  Storing this graph will require a larger\n"
	    "       initial STINGER allocation. Try reducing the number of vertices and/or edges per block in\n"
	    "       stinger_defs.h.  See the 'Handling Common Errors' section of the README.md for more\n"
	    "       information on how to do this.\n");
      abort();
    }
    OMP("omp parallel for")
      MTA ("mta assert nodep")
      MTASTREAMS ()MTA ("mta block schedule")
      for (size_t ki = 0; ki < k; ++ki)
        out[ki] = ebt0 + ki;
  }
}

/* }}} */

/* {{{ Internal utilities */

vindex_t
stinger_max_nv(stinger_t * S)
{
  return S->max_nv;
}

int64_t
stinger_max_num_etypes(stinger_t * S)
{
  return S->max_netypes;
}


/** @brief Calculate the largest active vertex ID
 *
 *  Finds the largest vertex ID whose in-degree and/or out-degree
 *  is greater than zero.
 *
 *  <em>NOTE:</em> If you are using this to obtain a
 *  value of "nv" for additional STINGER calls, you must add one to the 
 *  result.
 *
 *  @param S The STINGER data structure
 *  @return Largest active vertex ID
 */
uint64_t
stinger_max_active_vertex(const struct stinger * S) {
  uint64_t out = -1;
  OMP("omp parallel") {
    uint64_t local_max = 0;
    OMP("omp for")
    for(uint64_t i = 0; i < (S->max_nv); i++) {
      if((stinger_indegree_get(S, i) > 0 || stinger_outdegree_get(S, i) > 0) && 
	i > local_max) {
	local_max = i;
      }
    }
    OMP("omp critical") {
      if(local_max > out)
	out = local_max;
    }
  }
  return out;
}

/** @brief Calculate the number of active vertices
 *
 *  Counts the number of vertices whose in-degree is greater than zero and
 *  out-degree is greater than zero.
 *
 *  @param S The STINGER data structure
 *  @return Number of active vertices
 */
uint64_t
stinger_num_active_vertices(const struct stinger * S) {
  uint64_t out = 0;
  OMP("omp parallel for reduction(+:out)")
  for(uint64_t i = 0; i < (S->max_nv); i++) {
    if(stinger_indegree_get(S, i) > 0 || stinger_outdegree_get(S, i) > 0) {
      out++;
    }
  }
  return out;
}


const struct stinger_eb *
stinger_next_eb (const struct stinger *G,
                 const struct stinger_eb *eb_)
{
  const struct stinger_ebpool * ebpool = (const struct stinger_ebpool *)(G->storage + G->ebpool_start);
  return ebpool->ebpool + readff((uint64_t *)&eb_->next);
}

int64_t
stinger_eb_type (const struct stinger_eb * eb_)
{
  return eb_->etype;
}

int
stinger_eb_high (const struct stinger_eb *eb_)
{
  return eb_->high;
}

int
stinger_eb_is_blank (const struct stinger_eb *eb_, int k_)
{
  return eb_->edges[k_].neighbor < 0;
}

int64_t
stinger_eb_adjvtx (const struct stinger_eb * eb_, int k_)
{
  return eb_->edges[k_].neighbor;
}

int64_t
stinger_eb_weight (const struct stinger_eb * eb_, int k_)
{
  return eb_->edges[k_].weight;
}

int64_t
stinger_eb_ts (const struct stinger_eb * eb_, int k_)
{
  return eb_->edges[k_].timeRecent;
}

int64_t
stinger_eb_first_ts (const struct stinger_eb * eb_, int k_)
{
  return eb_->edges[k_].timeFirst;
}

/**
* @brief Count the number of edges in STINGER up to nv.
*
* @param S The STINGER data structure
* @param nv The maximum vertex ID to count.
*
* @return The number of edges in STINGER held by vertices 0 through nv-1
*/
int64_t
stinger_edges_up_to(const struct stinger * S, int64_t nv)
{
  uint64_t rtn = 0;
  OMP("omp parallel for reduction(+:rtn)")
    for (uint64_t i = 0; i < nv; i++) {
      rtn += stinger_outdegree_get(S, i);
    }
  return rtn;
}

/**
* @brief Count the total number of edges in STINGER.
*
* @param S The STINGER data structure
*
* @return The number of edges in STINGER
*/
int64_t
stinger_total_edges (const struct stinger * S)
{
  return stinger_edges_up_to (S, S->max_nv);
}

/**
* @brief Provide a fast upper bound on the total number of edges in STINGER.
*
* @param S The STINGER data structure
*
* @return An upper bound on the number of edges in STINGER
*/
int64_t
stinger_max_total_edges (const struct stinger * S)
{
  MAP_STING(S);
  return ebpool->ebpool_tail * STINGER_EDGEBLOCKSIZE;
}


/**
* @brief Calculate the total size of the active STINGER graph in memory.
*
* @param S The STINGER data structure
*
* @return The number of bytes currently in use by STINGER
*/
size_t
stinger_graph_size (const struct stinger *S)
{
  MAP_STING(S);
  int64_t num_edgeblocks = ebpool->ebpool_tail;;
  int64_t size_edgeblock = sizeof(struct stinger_eb);

  int64_t vertices_size = stinger_vertices_size_bytes(stinger_vertices_get(S));

  int64_t result = (num_edgeblocks * size_edgeblock) + vertices_size;

  return result;
}

/**
* @brief Calculates the required memory to allocate a STINGER given a set of parameters
*
* @param nv Number of vertices
* @param nebs Number of edge blocks
* @param netypes Number of edge types
* @param netypes Number of vertex types
*
* @return The STINGER size and start points for each sub-structure
*/
struct stinger_size_t calculate_stinger_size(int64_t nv, int64_t nebs, int64_t netypes, int64_t nvtypes) {
  struct stinger_size_t ret;

  uint64_t sz = 0;

  ret.vertices_start = 0;
  sz += stinger_vertices_size(nv);

  ret.physmap_start = sz;
  sz += stinger_physmap_size(nv);

  ret.ebpool_start = sz;
  sz += netypes * stinger_ebpool_size(nebs);

  ret.etype_names_start = sz;
  sz += stinger_names_size(netypes);

  ret.vtype_names_start = sz;
  sz += stinger_names_size(nvtypes);

  ret.ETA_start = sz;
  sz += netypes * stinger_etype_array_size(nebs);

  ret.size = sz;

  return ret;
}

void
stinger_print_eb(struct stinger_eb * eb) {
  printf(
    "EB VTX:  %ld\n"
    "  NEXT:    %ld\n"
    "  TYPE:    %ld\n"
    "  NUMEDGS: %ld\n"
    "  HIGH:    %ld\n"
    "  SMTS:    %ld\n"
    "  LGTS:    %ld\n"
    "  EDGES:\n",
    eb->vertexID, eb->next, eb->etype, eb->numEdges, eb->high, eb->smallStamp, eb->largeStamp);
  uint64_t j = 0;
  for (; j < eb->high && j < STINGER_EDGEBLOCKSIZE; j++) {
    printf("    TO: %s%ld WGT: %ld TSF: %ld TSR: %ld\n", 
      eb->edges[j].neighbor < 0 ? "x " : "  ", eb->edges[j].neighbor < 0 ? ~(eb->edges[j].neighbor) : eb->edges[j].neighbor, 
      eb->edges[j].weight, eb->edges[j].timeFirst, eb->edges[j].timeRecent);
  }
  if(j < STINGER_EDGEBLOCKSIZE) {
    printf("  ABOVE HIGH:\n");
    for (; j < STINGER_EDGEBLOCKSIZE; j++) {
      printf("    TO: %ld WGT: %ld TSF: %ld TSR: %ld\n", 
	eb->edges[j].neighbor, eb->edges[j].weight, eb->edges[j].timeFirst, eb->edges[j].timeRecent);
    }
  }
}

/**
* @brief Checks the STINGER metadata for inconsistencies.
*
* @param S The STINGER data structure
* @param NV The total number of vertices
*
* @return 0 on success; failure otherwise
*/
uint32_t
stinger_consistency_check (struct stinger *S, uint64_t NV)
{
  uint32_t returnCode = 0;
  uint64_t *inDegree = xcalloc (NV, sizeof (uint64_t));
  if (inDegree == NULL) {
    returnCode |= 0x00000001;
    return returnCode;
  }

  MAP_STING(S);
  struct stinger_eb * ebpool_priv = ebpool->ebpool;
  // check blocks
  OMP("omp parallel for reduction(|:returnCode)")
  MTA("mta assert nodep")
  for (uint64_t i = 0; i < NV; i++) {
    uint64_t curOutDegree = 0;
    const struct stinger_eb *curBlock = ebpool_priv + stinger_vertex_edges_get(vertices, i);
    while (curBlock != ebpool_priv) {
      if (curBlock->vertexID != i)
        returnCode |= 0x00000002;
      if (curBlock->high > STINGER_EDGEBLOCKSIZE)
        returnCode |= 0x00000004;

      int64_t numEdges = 0;
      int64_t smallStamp = INT64_MAX;
      int64_t largeStamp = INT64_MIN;

      uint64_t j = 0;
      for (; j < curBlock->high && j < STINGER_EDGEBLOCKSIZE; j++) {
        if (!stinger_eb_is_blank (curBlock, j)) {
          stinger_int64_fetch_add (&inDegree[stinger_eb_adjvtx (curBlock, j)], 1);
          curOutDegree++;
          numEdges++;
          if (stinger_eb_ts (curBlock, j) < smallStamp)
            smallStamp = stinger_eb_ts (curBlock, j);
          if (stinger_eb_first_ts (curBlock, j) < smallStamp)
            smallStamp = stinger_eb_first_ts (curBlock, j);
          if (stinger_eb_ts (curBlock, j) > largeStamp)
            largeStamp = stinger_eb_ts (curBlock, j);
          if (stinger_eb_first_ts (curBlock, j) > largeStamp)
            largeStamp = stinger_eb_first_ts (curBlock, j);
        }
      }
      if (numEdges && numEdges != curBlock->numEdges)
        returnCode |= 0x00000008;
      if (numEdges && largeStamp > curBlock->largeStamp)
        returnCode |= 0x00000010;
      if (numEdges && smallStamp < curBlock->smallStamp)
        returnCode |= 0x00000020;
      for (; j < STINGER_EDGEBLOCKSIZE; j++) {
        if (!(stinger_eb_is_blank (curBlock, j) ||
              (stinger_eb_adjvtx (curBlock, j) == 0
               && stinger_eb_weight (curBlock, j) == 0
               && stinger_eb_ts (curBlock, j) == 0
               && stinger_eb_first_ts (curBlock, j) == 0)))
          returnCode |= 0x00000040;
      }
      curBlock = ebpool_priv + curBlock->next;
    }

    if (curOutDegree != stinger_outdegree_get(S, i))
      returnCode |= 0x00000080;
  }

  OMP("omp parallel for reduction(|:returnCode)")
  MTA("mta assert nodep")
  for (uint64_t i = 0; i < NV; i++) {
    if (inDegree[i] != stinger_indegree_get(S,i))
      returnCode |= 0x00000100;
  }

  free (inDegree);

#if 0
  // check for self-edges and duplicate edges
  int64_t count_self = 0;
  int64_t count_duplicate = 0;

  int64_t * off = NULL;
  int64_t * ind = NULL;

  stinger_to_sorted_csr (S, NV, &off, &ind, NULL, NULL, NULL, NULL);

  MTA ("mta assert nodep")
  OMP ("omp parallel for reduction(+:count_self, count_duplicate)")
  for (int64_t k = 0; k < NV; k++)
  {
    int64_t myStart = off[k];
    int64_t myEnd = off[k+1];

    for (int64_t j = myStart; j < myEnd; j++)
    {
      if (ind[j] == k)
	count_self++;
      if (j < myEnd-1 && ind[j] == ind[j+1])
	count_duplicate++;
    }
  }

  free (ind);
  free (off);

  if (count_self != 0)
    returnCode |= 0x00000200;

  if (count_duplicate != 0)
    returnCode |= 0x00000400;
#endif

  return returnCode;
}

/**
* @brief Calculate statistics on edge block fragmentation in the graph
*
* @param S The STINGER data structure
* @param NV The total number of vertices
*
* @return Void
*/
void
stinger_fragmentation (struct stinger *S, uint64_t NV, struct stinger_fragmentation_t * stats)
{
  uint64_t numSpaces = 0;
  uint64_t numBlocks = 0;
  uint64_t numEdges = 0;
  uint64_t numEmptyBlocks = 0;

  MAP_STING(S);
  struct stinger_eb * ebpool_priv = ebpool->ebpool;
  OMP ("omp parallel for reduction(+:numSpaces, numBlocks, numEdges, numEmptyBlocks)")
  for (uint64_t i = 0; i < NV; i++) {
    const struct stinger_eb *curBlock = ebpool_priv + stinger_vertex_edges_get(vertices, i);

    while (curBlock != ebpool_priv) {
      uint64_t found = 0;

      if (curBlock->numEdges == 0) {
	numEmptyBlocks++;
      }
      else {
	/* for each edge in the current block */
	for (uint64_t j = 0; j < curBlock->high && j < STINGER_EDGEBLOCKSIZE; j++) {
	  if (stinger_eb_is_blank (curBlock, j)) {
	    numSpaces++;
	    found = 1;
	  }
	  else {
	    numEdges++;
	  }
	}
      }

      numBlocks += found;
      curBlock = ebpool_priv + curBlock->next;
    }
  }

  int64_t totalEdgeBlocks = ebpool->ebpool_tail;

  stats->num_empty_edges = numSpaces;
  stats->num_fragmented_blocks = numBlocks;
  stats->num_edges = numEdges;
  stats->edge_blocks_in_use = totalEdgeBlocks;
  stats->avg_number_of_edges = (double) numEdges / (double) (totalEdgeBlocks-numEmptyBlocks);
  stats->num_empty_blocks = numEmptyBlocks;

  double fillPercent = (double) numEdges / (double) (totalEdgeBlocks * STINGER_EDGEBLOCKSIZE);
  stats->fill_percent = fillPercent;
}

/* }}} */



/* {{{ Allocating and tearing down */

size_t
stinger_ebpool_size(int64_t nebs)
{
  return (sizeof(struct stinger_ebpool) + nebs * sizeof(struct stinger_eb));
}

size_t
stinger_etype_array_size(int64_t nebs)
{
  return (sizeof(struct stinger_etype_array) + nebs * sizeof(eb_index_t));
}

/** @brief Create a new STINGER data structure.
 *
 *  Allocates memory for a STINGER data structure.  If this is the first STINGER
 *  to be allocated, it also initializes the edge block pool.  Edge blocks are
 *  allocated and assigned for each value less than netypes.  The environment
 *  variable STINGER_MAX_MEMSIZE, if set to a number with optional size suffix,
 *  limits STINGER's maximum allocated size.
 *
 *  @return Pointer to struct stinger
 */
MTA ("mta inline")
struct stinger *stinger_new_full (struct stinger_config_t * config)
{
  int64_t nv      = config->nv      ? config->nv      : STINGER_DEFAULT_VERTICES;
  int64_t nebs    = config->nebs    ? config->nebs    : STINGER_DEFAULT_NEB_FACTOR * nv;
  int64_t netypes = config->netypes ? config->netypes : STINGER_DEFAULT_NUMETYPES;
  int64_t nvtypes = config->nvtypes ? config->nvtypes : STINGER_DEFAULT_NUMVTYPES;

  size_t max_memsize_env = stinger_max_memsize();

  const size_t memory_size = (config->memory_size == 0) ? max_memsize_env : config->memory_size;

  size_t i;
  int resized   = 0;
  struct stinger_size_t sizes;

  while (1) {
    sizes = calculate_stinger_size(nv, nebs, netypes, nvtypes);

    if(sizes.size > (((uint64_t)memory_size * 3) / 4)) {
      if (config->no_resize) {
        LOG_E("STINGER does not fit in memory.  no_resize set, so exiting.");
        exit(-1);
      }
      if(!resized) {
        LOG_W_A("Resizing stinger to fit into memory (detected as %ld)", memory_size);
      }
      resized = 1;

      nv    = (3*nv)/4;
      nebs  = STINGER_DEFAULT_NEB_FACTOR * nv;
    } else {
      break;
    }
  }

  struct stinger *G = xmalloc (sizeof(struct stinger) + sizes.size);

  xzero(G, sizeof(struct stinger) + sizes.size);

  G->max_nv       = nv;
  G->max_neblocks = nebs;
  G->max_netypes  = netypes;
  G->max_nvtypes  = nvtypes;

  G->length = sizes.size;
  G->vertices_start = sizes.vertices_start;
  G->physmap_start = sizes.physmap_start;
  G->etype_names_start = sizes.etype_names_start;
  G->vtype_names_start = sizes.vtype_names_start;
  G->ETA_start = sizes.ETA_start;
  G->ebpool_start = sizes.ebpool_start;

  MAP_STING(G);

  int64_t zero = 0;
  stinger_vertices_init(vertices, nv);
  stinger_physmap_init(physmap, nv);
  stinger_names_init(etype_names, netypes);
  if (!config->no_map_none_etype) {
    stinger_names_create_type(etype_names, "None", &zero);
  }
  stinger_names_init(vtype_names, nvtypes);
  if (!config->no_map_none_vtype) {
    stinger_names_create_type(vtype_names, "None", &zero);
  }

  ebpool->ebpool_tail = 1;
  ebpool->is_shared = 0;

  OMP ("omp parallel for")
  MTA ("mta assert parallel")
  MTASTREAMS ()
  for (i = 0; i < netypes; ++i) {
    ETA(G,i)->length = nebs;
    ETA(G,i)->high = 0;
  }

  return G;
}


/** @brief Create a new STINGER data structure.
 *
 *  Allocates memory for a STINGER data structure.  If this is the first STINGER
 *  to be allocated, it also initializes the edge block pool.  Edge blocks are
 *  allocated and assigned for each value less than netypes.
 *
 *  @return Pointer to struct stinger
 */
MTA ("mta inline")
struct stinger *stinger_new (void)
{
  struct stinger_config_t * config = (struct stinger_config_t *)xcalloc(1,sizeof(struct stinger_config_t));

  struct stinger * s = stinger_new_full(config);
  xfree(config);

  return s;
}

/** @brief Free memory allocated to a particular STINGER instance.
 *
 *  Frees the ETA pointers for each edge type, the LVA, and the struct stinger
 *  itself.  Does not actually free any edge blocks, as there may be other
 *  active STINGER instances.
 *
 *  @param S The STINGER data structure
 *  @return NULL on success
 */
struct stinger *
stinger_free (struct stinger *S)
{
  size_t i;
  if (!S)
    return S;

  free (S);
  return NULL;
}

/** @brief Free the STINGER data structure and all edge blocks.
 *
 *  Free memory allocated to the specified STINGER data structure.  Also frees
 *  the STINGER edge block pool, effectively ending all STINGER operations globally.
 *  Only call this function if you are done using STINGER entirely.
 *
 *  @param S The STINGER data structure
 *  @return NULL on success
 */
struct stinger *
stinger_free_all (struct stinger *S)
{
  struct stinger *out;
  out = stinger_free (S);
  return out;
}

/* TODO inspect possibly move out with other EB POOL stuff */
MTA ("mta expect parallel context")
static eb_index_t new_eb (struct stinger * S, int64_t etype, int64_t from)
{
  MAP_STING(S);
  size_t k;
  eb_index_t out = 0;
  get_from_ebpool (S, &out, 1);
  struct stinger_eb * block = ebpool->ebpool + out;
  assert (block != ebpool->ebpool);
  xzero (block, sizeof (*block));
  block->etype = etype;
  block->vertexID = from;
  block->smallStamp = INT64_MAX;
  block->largeStamp = INT64_MIN;
  return out;
}

MTA ("mta expect parallel context")
void
new_ebs (struct stinger * S, eb_index_t *out, size_t neb, int64_t etype,
         int64_t from)
{
  if (neb < 1)
    return;
  get_from_ebpool (S, out, neb);

  MAP_STING(S);

  OMP ("omp parallel for")
    //MTA("mta assert nodep")
    MTASTREAMS ()MTA ("mta block schedule")
    //MTA("mta parallel single processor")
    for (size_t i = 0; i < neb; ++i) {
      struct stinger_eb * block = ebpool->ebpool + out[i];
      xzero (block, sizeof (*block));
      block->etype = etype;
      block->vertexID = from;
      block->smallStamp = INT64_MAX;
      block->largeStamp = INT64_MIN;
    }
}

MTA ("mta expect serial context")
void
new_blk_ebs (eb_index_t *out, const struct stinger *restrict G,
             const int64_t nvtx, const size_t * restrict blkoff,
             const int64_t etype)
{
  size_t neb;
  if (nvtx < 1)
    return;
  neb = blkoff[nvtx];
  get_from_ebpool (G,out, neb);

  MAP_STING(G);

  OMP ("omp parallel for")
    MTA ("mta assert nodep")
    MTASTREAMS ()MTA ("mta block schedule")
    for (size_t k = 0; k < neb; ++k) {
      struct stinger_eb * block = ebpool->ebpool + out[k];
      xzero (block, sizeof (*block));
      block->etype = etype;
      block->smallStamp = INT64_MAX;
      block->largeStamp = INT64_MIN;
    }

  OMP ("omp parallel for")
    MTA ("mta assert nodep")
    MTASTREAMS ()MTA ("mta interleave schedule")
    for (int64_t v = 0; v < nvtx; ++v) {
      const int64_t from = v;
      const size_t blkend = blkoff[v + 1];
      MTA ("mta assert nodep")
        for (size_t k = blkoff[v]; k < blkend; ++k)
          ebpool->ebpool[out[k]].vertexID = from;
      if (blkend)
        MTA ("mta assert nodep")
          for (size_t k = blkoff[v]; k < blkend - 1; ++k)
            ebpool->ebpool[out[k]].next = out[k + 1];
    }
}

/* }}} */

MTA ("mta inline")
void
push_ebs (struct stinger *G, size_t neb,
          eb_index_t * restrict eb)
{
  int64_t etype, place;
  assert (G);
  assert (eb);

  if (!neb)
    return;

  MAP_STING(G);
  etype = ebpool->ebpool[eb[0]].etype;
  assert (etype >= 0);
  assert (etype < G->max_netypes);

  place = stinger_int64_fetch_add (&(ETA(G,etype)->high), neb);

  eb_index_t *blocks;
  blocks = ETA(G,etype)->blocks;

  MTA ("mta assert nodep")
  for (int64_t k = 0; k < neb; ++k)
    blocks[place + k] = eb[k];
}

MTA ("mta inline")
struct curs
etype_begin (stinger_t * S, int64_t v, int etype)
{
  MAP_STING(S);
  struct curs out;
  assert (vertices);
  out.eb = stinger_vertex_edges_get(vertices,v);
  out.loc = stinger_vertex_edges_pointer_get(vertices,v);
  while (out.eb && ebpool->ebpool[out.eb].etype != etype) {
    out.loc = &(ebpool->ebpool[out.eb].next);
    out.eb = readff((uint64_t *)&(ebpool->ebpool[out.eb].next));
  }
  return out;
}

MTA ("mta inline")
void
update_edge_data (struct stinger * S, struct stinger_eb *eb,
                  uint64_t index, int64_t neighbor, int64_t weight,
		  int64_t ts, int64_t operation)
{
  struct stinger_edge * e = eb->edges + index;

  /* insertion */
  if (neighbor >= 0) {
    switch (operation) {
      case EDGE_WEIGHT_SET:
	e->weight = weight;
	break;
      case EDGE_WEIGHT_INCR:
	stinger_int64_fetch_add(&(e->weight), weight);
	break;
    }
    /* is this a new edge */
    if (e->neighbor < 0 || index >= eb->high) {
      e->neighbor = neighbor;

      /* only edge in block? - assuming we have block effectively locked */
      if(stinger_int64_fetch_add(&eb->numEdges, 1) == 0) {
	eb->smallStamp = ts;
	eb->largeStamp = ts;
      }

      /* register new edge */
      stinger_outdegree_increment_atomic(S, eb->vertexID, 1);
      stinger_indegree_increment_atomic(S, neighbor, 1);

      if (index >= eb->high)
	eb->high = index + 1;

      writexf(&e->timeFirst, ts);
    }

    /* check metadata and update - lock metadata for safety */
    if (ts < readff(&eb->smallStamp) || ts > eb->largeStamp) {
      int64_t smallStamp = readfe(&eb->smallStamp);
      if (ts < smallStamp)
	smallStamp = ts;
      if (ts > eb->largeStamp)
	eb->largeStamp = ts;
      writeef(&eb->smallStamp, smallStamp);
    }

    e->timeRecent = ts;

  } else if(e->neighbor >= 0) {
    /* are we deleting an edge */
    stinger_outdegree_increment_atomic(S, eb->vertexID, -1);
    stinger_indegree_increment_atomic(S, e->neighbor, -1);
    stinger_int64_fetch_add (&(eb->numEdges), -1);
    e->neighbor = neighbor;
  } 

  /* we always do this to update weight and  unlock the edge if needed */
}

/** @brief Insert a directed edge.
 *
 *  Inserts a typed, directed edge.  First timestamp is set, if the edge is
 *  new.  Recent timestamp is updated.  Weight is set to specified value regardless.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param weight Edge weight
 *  @param timestamp Edge timestamp
 *  @return 1 if edge is inserted successfully for the first time, 0 if edge is already found and updated, -1 if error.
 */
MTA ("mta inline") MTA("mta serial")
int
stinger_insert_edge (struct stinger *G,
                     int64_t type, int64_t from, int64_t to,
                     int64_t weight, int64_t timestamp)
{
  /* Do *NOT* call this concurrently with different edge types. */
  STINGERASSERTS ();
  MAP_STING(G);

  struct curs curs;
  struct stinger_eb *tmp;
  struct stinger_eb *ebpool_priv = ebpool->ebpool;

  curs = etype_begin (G, from, type);
  /*
  Possibilities:
  1: Edge already exists and only needs updated.
  2: Edge does not exist, fits in an existing block.
  3: Edge does not exist, needs a new block.
  */

  /* 1: Check if the edge already exists. */
  for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff((uint64_t *)&tmp->next)) {
    if(type == tmp->etype) {
      size_t k, endk;
      endk = tmp->high;

      for (k = 0; k < endk; ++k) {
        if (to == tmp->edges[k].neighbor) {
          update_edge_data (G, tmp, k, to, weight, timestamp, EDGE_WEIGHT_SET);
          return 0;
        }
      }
    }
  }

  while (1) {
    eb_index_t * block_ptr = curs.loc;
    curs.eb = readff((uint64_t *)curs.loc);
    /* 2: The edge isn't already there.  Check for an empty slot. */
    for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff((uint64_t *)&tmp->next)) {
      if(type == tmp->etype) {
        size_t k, endk;
        endk = tmp->high;

        for (k = 0; k < STINGER_EDGEBLOCKSIZE; ++k) {
          int64_t myNeighbor = tmp->edges[k].neighbor;
          if (to == myNeighbor && k < endk) {
            update_edge_data (G, tmp, k, to, weight, timestamp, EDGE_WEIGHT_SET);
            return 0;
          }

          if (myNeighbor < 0 || k >= endk) {
            int64_t timefirst = readfe ( &(tmp->edges[k].timeFirst) );
            int64_t thisEdge = tmp->edges[k].neighbor;
            endk = tmp->high;

            if (thisEdge < 0 || k >= endk) {
              update_edge_data (G, tmp, k, to, weight, timestamp, EDGE_WEIGHT_SET);
              return 1;
            } else if (to == thisEdge) {
              update_edge_data (G, tmp, k, to, weight, timestamp, EDGE_WEIGHT_SET);
              writexf ( &(tmp->edges[k].timeFirst), timefirst);
              return 0;
            } else {
              writexf ( &(tmp->edges[k].timeFirst), timefirst);
            }
          }
        }
      }
      block_ptr = &(tmp->next);
    }

    /* 3: Needs a new block to be inserted at end of list. */
    eb_index_t old_eb = readfe ((uint64_t *)block_ptr );
    if (!old_eb) {
      eb_index_t newBlock = new_eb (G, type, from);
      if (newBlock == 0) {
        writeef ((uint64_t *)block_ptr, (uint64_t)old_eb);
        return -1;
      } else {
        update_edge_data (G, ebpool_priv + newBlock, 0, to, weight, timestamp, EDGE_WEIGHT_SET);
        ebpool_priv[newBlock].next = 0;
        push_ebs (G, 1, &newBlock);
      }
      writeef ((uint64_t *)block_ptr, (uint64_t)newBlock);
      return 1;
    }
    writeef ((uint64_t *)block_ptr, (uint64_t)old_eb);
  }
}

/** @brief Returns the out-degree of a vertex for a given edge type
 *
 *  @param S The STINGER data structure
 *  @param i Logical vertex ID
 *  @param type Edge type
 *  @return Out-degree of vertex i with type
 */
int64_t
stinger_typed_outdegree (const struct stinger * S, int64_t i, int64_t type) {
  MAP_STING(S);
  int64_t out = 0;
  struct curs curs;
  struct stinger_eb *tmp;
  struct stinger_eb *ebpool_priv = ebpool->ebpool;

  curs = etype_begin (S, i, type);

  for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff((uint64_t *)&tmp->next)) {
    if(type == tmp->etype) {
      out += tmp->numEdges;
    }
  }
  return out;
}

/** @brief Increments a directed edge.
 *
 *  Increments the weight of a typed, directed edge.
 *  Recent timestamp is updated.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param weight Edge weight
 *  @param timestamp Edge timestamp
 *  @return 1
 */
MTA ("mta inline")
int
stinger_incr_edge (struct stinger *G,
                   int64_t type, int64_t from, int64_t to,
                   int64_t weight, int64_t timestamp)
{
  /* Do *NOT* call this concurrently with different edge types. */
  STINGERASSERTS ();
  MAP_STING(G);

  struct curs curs;
  struct stinger_eb *tmp;
  struct stinger_eb *ebpool_priv = ebpool->ebpool;

  curs = etype_begin (G, from, type);
  /*
  Possibilities:
  1: Edge already exists and only needs updated.
  2: Edge does not exist, fits in an existing block.
  3: Edge does not exist, needs a new block.
  */

  /* 1: Check if the edge already exists. */
  for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff((uint64_t *)&tmp->next)) {
    if(type == tmp->etype) {
      size_t k, endk;
      endk = tmp->high;

      for (k = 0; k < endk; ++k) {
	if (to == tmp->edges[k].neighbor) {
	  update_edge_data (G, tmp, k, to, weight, timestamp, EDGE_WEIGHT_INCR);
	  return 0;
	}
      }
    }
  }

  while (1) {
    eb_index_t * block_ptr = curs.loc;
    curs.eb = readff((uint64_t *)curs.loc);
    /* 2: The edge isn't already there.  Check for an empty slot. */
    for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff((uint64_t *)&tmp->next)) {
      if(type == tmp->etype) {
	size_t k, endk;
	endk = tmp->high;

	for (k = 0; k < STINGER_EDGEBLOCKSIZE; ++k) {
	  int64_t myNeighbor = tmp->edges[k].neighbor;
	  if (to == myNeighbor && k < endk) {
	    update_edge_data (G, tmp, k, to, weight, timestamp, EDGE_WEIGHT_INCR);
	    return 0;
	  }

	  if (myNeighbor < 0 || k >= endk) {
	    int64_t timefirst = readfe ( &(tmp->edges[k].timeFirst) );
	    int64_t thisEdge = tmp->edges[k].neighbor;
	    endk = tmp->high;

	    if (thisEdge < 0 || k >= endk) {
	      update_edge_data (G, tmp, k, to, weight, timestamp, EDGE_WEIGHT_SET);
	      return 1;
	    } else if (to == thisEdge) {
	      update_edge_data (G, tmp, k, to, weight, timestamp, EDGE_WEIGHT_INCR);
	      writexf ( &(tmp->edges[k].timeFirst), timefirst);
	      return 0;
	    } else {
	      writexf ( &(tmp->edges[k].timeFirst), timefirst);
	    }
	  }
	}
      }
      block_ptr = &(tmp->next);
    }

    /* 3: Needs a new block to be inserted at end of list. */
    eb_index_t old_eb = readfe ((uint64_t *)block_ptr );
    if (!old_eb) {
      eb_index_t newBlock = new_eb (G, type, from);
      if (newBlock == 0) {
	writeef ((uint64_t *)block_ptr, (uint64_t)old_eb);
	return -1;
      } else {
	update_edge_data (G, ebpool_priv + newBlock, 0, to, weight, timestamp, EDGE_WEIGHT_SET);
	ebpool_priv[newBlock].next = 0;
	push_ebs (G, 1, &newBlock);
      }
      writeef ((uint64_t *)block_ptr, (uint64_t)newBlock);
      return 1;
    }
    writeef ((uint64_t *)block_ptr, (uint64_t)old_eb);
  }
}

/** @brief Insert an undirected edge.
 *
 *  Inserts a typed, undirected edge.  First timestamp is set, if the edge is
 *  new.  Recent timestamp is updated.  Weight is set to specified value regardless.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param weight Edge weight
 *  @param timestamp Edge timestamp
 *  @return Number of edges inserted successfully
 */
MTA ("mta inline")
int
stinger_insert_edge_pair (struct stinger *G,
                          int64_t type, int64_t from, int64_t to,
                          int64_t weight, int64_t timestamp)
{
  STINGERASSERTS();

  int rtn = stinger_insert_edge (G, type, from, to, weight, timestamp);
  int rtn2 = stinger_insert_edge (G, type, to, from, weight, timestamp);

  /* Check if either returned -1 */
  if(rtn < 0 || rtn2 < 0)
    return -1;
  else
    return rtn | (rtn2 << 1);
}

/** @brief Increments an undirected edge.
 *
 *  Increments the weight of a typed, undirected edge.
 *  Recent timestamp is updated.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param weight Edge weight
 *  @param timestamp Edge timestamp
 *  @return 1
 */
MTA ("mta inline")
int
stinger_incr_edge_pair (struct stinger *G,
                        int64_t type, int64_t from, int64_t to,
                        int64_t weight, int64_t timestamp)
{
  STINGERASSERTS();

  int rtn = stinger_incr_edge (G, type, from, to, weight, timestamp);
  int rtn2 = stinger_incr_edge (G, type, to, from, weight, timestamp);

  /* Check if either returned -1 */
  if(rtn < 0 || rtn2 < 0)
    return -1;
  else
    return rtn | (rtn2 << 1);
}

/** @brief Removes a directed edge.
 *
 *  Remove a typed, directed edge.
 *  Note: Do not call this function concurrently with the same source vertex,
 *  even for different edge types.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @return 1 on success, 0 if the edge is not found.
 */
MTA ("mta inline") MTA ("mta serial")
int
stinger_remove_edge (struct stinger *G,
                     int64_t type, int64_t from, int64_t to)
{
  /* Do *NOT* call this concurrently with different edge types. */
  STINGERASSERTS ();
  MAP_STING(G);

  struct curs curs;
  struct stinger_eb *tmp;
  struct stinger_eb *ebpool_priv = ebpool->ebpool;

  curs = etype_begin (G, from, type);

  for (tmp = ebpool_priv + curs.eb; tmp != ebpool_priv; tmp = ebpool_priv + readff((uint64_t *)&tmp->next)) {
    if(type == tmp->etype) {
      size_t k, endk;
      endk = tmp->high;

      for (k = 0; k < endk; ++k) {
	if (to == tmp->edges[k].neighbor) {
	  int64_t weight = readfe (&(tmp->edges[k].weight));
	  if(to == tmp->edges[k].neighbor) {
	    update_edge_data (G, tmp, k, ~to, weight, 0, EDGE_WEIGHT_SET);
	    return 1;
	  } else {
	    writeef((uint64_t *)&(tmp->edges[k].weight), (uint64_t)weight);
	  }
	  return 0;
	}
      }
    }
  }
  return -1;
}

/** @brief Removes an undirected edge.
 *
 *  Remove a typed, undirected edge.
 *  Note: Do not call this function concurrently with the same source vertex,
 *  even for different edge types.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @return 1 on success, 0 if the edge is not found.
 */
MTA ("mta inline")
int
stinger_remove_edge_pair (struct stinger *G,
                          int64_t type, int64_t from, int64_t to)
{
  STINGERASSERTS();

  int rtn = stinger_remove_edge (G, type, from, to);
  int rtn2 = stinger_remove_edge (G, type, to, from);

  /* Check if either returned -1 */
  if(rtn < 0 || rtn2 < 0)
    return -1;
  else
    return rtn | (rtn2 << 1);
}

MTA("mta parallel default")

/** @brief Initializes an empty STINGER with a graph in CSR format.
 *
 *  Takes an edge list in CSR format, with weight and timestamps, and initializes
 *  an empty STINGER based on the input graph.  All edges being ingested must be
 *  of a single edge type.
 *
 *  @param G STINGER data structure
 *  @param nv Number of vertices
 *  @param etype Edge type
 *  @param off_in Array of length nv+1 containing the adjacency offset for each vertex
 *  @param phys_adj_in Array containing the destination vertex of each edge
 *  @param weight_in Array containing integer weight of each edge
 *  @param ts_in Array containing recent timestamp of edge edge (or NULL)
 *  @param first_ts_in Array containing the first timestamp of each edge (or NULL)
 *  @param single_ts Value for timestamps if either of the above is NULL
 *  @return Void
 */
MTA ("mta inline")
void
stinger_set_initial_edges (struct stinger *G,
                           const size_t nv,
                           const int64_t etype,
                           const int64_t * off_in,
                           const int64_t * phys_adj_in,
                           const int64_t * weight_in,
                           const int64_t * ts_in,
                           const int64_t * first_ts_in,
                           const int64_t single_ts
                           /* if !ts or !first_ts */ )
{
  const int64_t * restrict off = off_in;
  const int64_t * restrict phys_adj = phys_adj_in;
  const int64_t * restrict weight = weight_in;
  const int64_t * restrict ts = ts_in;
  const int64_t * restrict first_ts = first_ts_in;

  size_t nblk_total = 0;
  size_t * restrict blkoff;
  eb_index_t * restrict block;

  assert (G);
  MAP_STING(G);

  blkoff = xcalloc (nv + 1, sizeof (*blkoff));
  OMP ("omp parallel for")
  for (int64_t v = 0; v < nv; ++v) {
    const int64_t deg = off[v + 1] - off[v];
    blkoff[v + 1] = (deg + STINGER_EDGEBLOCKSIZE - 1) / STINGER_EDGEBLOCKSIZE;
  }

  for (int64_t v = 2; v <= nv; ++v) {
    blkoff[v] += blkoff[v - 1];
  }
  nblk_total = blkoff[nv];

  block = xcalloc (nblk_total, sizeof (*block));
  OMP ("omp parallel for")
  MTA ("mta assert nodep") MTASTREAMS ()
  for (int64_t v = 0; v < nv; ++v) {
    const int64_t from = v;
    const int64_t deg = off[v + 1] - off[v];
    if (deg) {
      stinger_vertex_outdegree_increment_atomic(vertices, from, deg);
    }
  }

  new_blk_ebs (&block[0], G, nv, blkoff, etype);

  /* XXX: AUGH! I cannot find what really is blocking parallelization. */
  MTA ("mta assert parallel")
  MTA ("mta dynamic schedule")
  OMP ("omp parallel for")
  MTA ("mta assert noalias *block")
  MTA ("mta assert nodep *block")
  MTA ("mta assert noalias *G")
  MTA ("mta assert nodep *G")
  MTA ("mta assert nodep *phys_adj")
  MTA ("mta assert nodep *vertices")
  MTA ("mta assert nodep *blkoff")
  for (int64_t v = 0; v < nv; ++v) {
    const size_t nextoff = off[v + 1];
    size_t kgraph = off[v];
    MTA ("mta assert may reorder kgraph") int64_t from;

    from = v;

    // Need to assert this loop is not to be parallelized!
    MTA ("mta assert nodep *block")
    MTA ("mta assert nodep *vertices")
    for (size_t kblk = blkoff[v]; kblk < blkoff[v + 1]; ++kblk) {
      size_t n_to_copy, voff;
      struct stinger_edge * restrict edge;
      struct stinger_eb * restrict eb;
      int64_t tslb = INT64_MAX, tsub = 0;

      //voff = stinger_size_fetch_add (&kgraph, STINGER_EDGEBLOCKSIZE);
      {
	voff = kgraph;
	kgraph += STINGER_EDGEBLOCKSIZE;
      }

      n_to_copy = STINGER_EDGEBLOCKSIZE;
      if (voff + n_to_copy >= nextoff)
	n_to_copy = nextoff - voff;

      eb = ebpool->ebpool + block[kblk];
      edge = &eb->edges[0];

      /* XXX: remove the next two asserts once the outer is unlocked. */
      MTA ("mta assert nodep")
      MTASTREAMS ()MTA ("mta assert nodep *phys_adj")
      MTA ("mta assert nodep *edge")
      MTA ("mta assert nodep *vertices")
      for (size_t i = 0; i < n_to_copy; ++i) {
	const int64_t to = phys_adj[voff + i];
#if defined(__MTA__)
	stinger_vertex_indegree_increment_atomic(vertices, to, 1);
#else
	// Ugh. The MTA compiler can't cope with the inlining.
	stinger_vertex_indegree_increment_atomic(vertices, to, 1);
#endif
	/* XXX: The next statements block parallelization
	   of the outer loop. */
	edge[i].neighbor = to;
	edge[i].weight = weight[voff + i];
	edge[i].timeRecent = ts ? ts[voff + i] : single_ts;
	edge[i].timeFirst = first_ts ? first_ts[voff + i] : single_ts;
	//assert (edge[i].timeRecent >= edge[i].timeFirst);
      }

      if (ts || first_ts) {
	/* XXX: remove the next two asserts once the outer is unlocked. */
	MTA ("mta assert nodep")
	MTASTREAMS ()MTA ("mta assert nodep *edge")
	for (size_t i = 0; i < n_to_copy; ++i) {
	  if (edge[i].timeFirst < tslb) {
	    tslb = edge[i].timeFirst;
	  }
	  if (edge[i].timeRecent < tslb) {
	    tslb = edge[i].timeRecent;
	  }
	  if (edge[i].timeFirst > tsub) {
	    tsub = edge[i].timeFirst;
	  }
	  if (edge[i].timeRecent > tsub) {
	    tsub = edge[i].timeRecent;
	  }
	}
      } else {
	tslb = tsub = single_ts;
      }

      eb->smallStamp = tslb;
      eb->largeStamp = tsub;
      eb->numEdges = n_to_copy;
      eb->high = n_to_copy;
    }


    /* At this point, block[blkoff[v]] is the head of a linked
       list holding all the blocks of edges of EType from vertex
       v.  Insert into the graph.  */

    if (blkoff[v] != blkoff[v + 1]) {
      ebpool->ebpool[block[blkoff[v+1]-1]].next = stinger_vertex_edges_get(vertices, from);
      stinger_vertex_edges_set(vertices, from, block[blkoff[v]]);
    } 
  }

  /* Insert into the edge type array */
  push_ebs (G, nblk_total, block);

  free (block);
  free (blkoff);
}

/** @brief Copy typed incoming adjacencies of a vertex into a buffer
 *
 *  For a given edge type, adjacencies of the specified vertex are copied into
 *  the user-provided buffer up to the length of the buffer.  These are the
 *  incoming edges for which a vertex is a destination.  Note that this operation
 *  may be very expensive on most platforms.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param v Source vertex ID
 *  @param outlen Number of adjacencies copied
 *  @param out Buffer to hold adjacencies
 *  @param max_outlen Length of out[] and recent[]
 *  @return Void
 */
MTA ("mta inline")
void
stinger_gather_typed_predecessors (const struct stinger *G,
				   int64_t type,
				   int64_t v,
				   size_t * outlen,
                                   int64_t * out,
                                   size_t max_outlen)
{
  size_t kout = 0;

  assert (G);

  STINGER_PARALLEL_FORALL_EDGES_BEGIN(G, type) {
    const int64_t u = STINGER_EDGE_SOURCE;
    const int64_t d = STINGER_EDGE_DEST;
    if (v == d) {
      size_t where = stinger_size_fetch_add (&kout, 1);
      if (where < max_outlen) {
        out[where] = u;
      }
    }
  } STINGER_PARALLEL_FORALL_EDGES_END();

  *outlen = kout;               /* May be longer than max_outlen. */
}

/** @brief Copy adjacencies of a vertex into a buffer with optional metadata
 *
 *  Adjacencies of the specified vertex are copied into the user-provided buffer(s) 
 *  up to the length of the buffer(s) specified by max_outlen.  All buffers should 
 *  be at least max_outlen or NULL.
 *
 *  @param G The STINGER data structure
 *  @param v Source vertex ID
 *  @param outlen Number of adjacencies copied
 *  @param out Buffer to hold adjacencies
 *  @param weight OPTIONAL Buffer to hold edge weights
 *  @param timefirst OPTIONAL Buffer to hold first timestamps
 *  @param timerecent OPTIONAL Buffer to hold recent timestamps
 *  @param type OPTIONAL Buffer to hold edge types
 *  @param max_outlen Length of out[] and any optional buffers provided
 *  @return Void
 */
MTA ("mta inline")
void
stinger_gather_successors (const struct stinger *G,
                                          int64_t v,
                                          size_t * outlen,
                                          int64_t * out,
					  int64_t * weight,
					  int64_t * timefirst,
                                          int64_t * timerecent,
					  int64_t * type,
                                          size_t max_outlen)
{
  size_t kout = 0;

  assert (G);

  STINGER_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(G, v) {
    const int64_t n = STINGER_EDGE_DEST;
    const int64_t w = STINGER_EDGE_WEIGHT;
    const int64_t tf = STINGER_EDGE_TIME_FIRST;
    const int64_t tr = STINGER_EDGE_TIME_RECENT;
    const int64_t t = STINGER_EDGE_TYPE;
    if (n >= 0) {
      size_t where = stinger_size_fetch_add (&kout, 1);
      if (where < max_outlen) {
        out[where] = n;
        if(weight) weight[where] = w;
        if(timefirst) timefirst[where] = tf;
        if(timerecent) timerecent[where] = tr;
        if(type) type[where] = t;
      }
    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_VTX_END();

  *outlen = kout;               /* May be longer than max_outlen. */
}

/** @brief Copy typed adjacencies of a vertex into a buffer
 *
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
MTA ("mta inline")
void
stinger_gather_typed_successors (const struct stinger *G, int64_t type,
                                 int64_t v, size_t * outlen,
                                 int64_t * out, size_t max_outlen)
{
  size_t kout = 0;

  assert (G);

  if (v < 0) {
    *outlen = 0;
    return;
  }

  STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(G, type, v) {
    size_t where = stinger_size_fetch_add (&kout, 1);
    if (where < max_outlen)
      out[where] = STINGER_EDGE_DEST;
  } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();

  *outlen = kout;               /* May be longer than max_outlen. */
}

/** @brief Determines if a vertex has an edge of a given type
 *
 *  Searches the adjacencies of a specified vertex for an edge of the given type.
 *
 *  @param G The STINGER data structure
 *  @param type Edge type
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @return 1 if found; 0 otherwise
 */
MTA ("mta inline")
int
stinger_has_typed_successor (const struct stinger *G,
    int64_t type, int64_t from, int64_t to)
{
  STINGERASSERTS ();

  int rtn = 0;

  STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(G,type,from) {
    if (STINGER_EDGE_DEST == to) {
      stinger_int_fetch_add(&rtn, 1);
    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  return (rtn > 0 ? 1 : 0); 
}

/** @brief Get the weight of a given edge.
 *
 *  Finds a specified edge of a given type by source and destination vertex ID
 *  and returns the current edge weight.  Remember, edges may have different
 *  weights in different directions.
 *
 *  @param G The STINGER data structure
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param type Edge type
 *  @return Edge weight
 */
MTA ("mta inline")
int64_t
stinger_edgeweight (const struct stinger * G,
                    int64_t from, int64_t to, int64_t type)
{
  STINGERASSERTS ();

  int rtn = 0;

  STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(G,type,from) {
    if (STINGER_EDGE_DEST == to) {
      rtn = STINGER_EDGE_WEIGHT;
    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  return rtn;
}

/* TODO revisit this function
 * XXX how to handle in parallel?
 * BUG block meta not handled
 * */
/** @brief Set the weight of a given edge.
 *
 *  Finds a specified edge of a given type by source and destination vertex ID
 *  and sets the current edge weight.  Remember, edges may have different
 *  weights in different directions.
 *
 *  @param G The STINGER data structure
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param type Edge type
 *  @param weight Edge weight to set
 *  @return 1 on success; 0 otherwise
 */
MTA ("mta inline")
int
stinger_set_edgeweight (struct stinger *G,
                        int64_t from, int64_t to, int64_t type,
                        int64_t weight)
{
  STINGERASSERTS ();

  int rtn = 0;

  STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(G,type,from) {
    if (STINGER_EDGE_DEST == to) {
      STINGER_EDGE_WEIGHT = weight;
      rtn = 1;
    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  return rtn;
}

/** @brief Get the first timestamp of a given edge.
 *
 *  Finds a specified edge of a given type by source and destination vertex ID
 *  and returns the first timestamp.  Remember, edges may have different
 *  timestamps in different directions.
 *
 *  @param G The STINGER data structure
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param type Edge type
 *  @return First timestamp of edge; -1 if edge does not exist
 */
MTA ("mta inline")
int64_t
stinger_edge_timestamp_first (const struct stinger * G,
                              int64_t from, int64_t to, int64_t type)
{
  STINGERASSERTS ();

  int rtn = -1;

  STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(G,type,from) {
    if (STINGER_EDGE_DEST == to) {
      rtn = STINGER_EDGE_TIME_FIRST;
    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  return rtn;
}

/** @brief Get the recent timestamp of a given edge.
 *
 *  Finds a specified edge of a given type by source and destination vertex ID
 *  and returns the recent timestamp.  Remember, edges may have different
 *  timestamps in different directions.
 *
 *  @param G The STINGER data structure
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param type Edge type
 *  @return Recent timestamp of edge; -1 if edge does not exist
 */
MTA ("mta inline")
int64_t
stinger_edge_timestamp_recent (const struct stinger * G,
                               int64_t from, int64_t to, int64_t type)
{
  STINGERASSERTS ();

  int rtn = -1;

  STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(G,type,from) {
    if (STINGER_EDGE_DEST == to) {
      rtn = STINGER_EDGE_TIME_RECENT;
    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  return rtn;
}

/* TODO revisit this function
 * XXX how to handle in parallel?
 * BUG block meta not handled
 * */
/** @brief Update the recent timestamp of an edge
 *
 *  Finds a specified edge of a given type by source and destination vertex ID
 *  and updates the recent timestamp to the specified value.
 *
 *  @param G The STINGER data structure
 *  @param from Source vertex ID
 *  @param to Destination vertex ID
 *  @param type Edge type
 *  @param timestamp Timestamp to set recent
 *  @return 1 on success; 0 if failure
 */
MTA ("mta inline")
int
stinger_edge_touch (struct stinger *G,
    int64_t from, int64_t to, int64_t type,
    int64_t timestamp)
{
  STINGERASSERTS ();

  int rtn = 0;

  STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(G,type,from) {
    if (STINGER_EDGE_DEST == to) {
      STINGER_EDGE_TIME_RECENT = timestamp;
      rtn = 1;
    }
  } STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END();
  return rtn;
}

/* TODO - add doxygen and insert into stinger.h */
MTA ("mta inline") int64_t
stinger_count_outdeg (struct stinger * G, int64_t v)
{
  int64_t nactive_edges = 0, neb = 0;

  STINGER_FORALL_EB_BEGIN (G, v, eb) {
    const size_t eblen = stinger_eb_high (eb);
    OMP ("omp parallel for")
      MTA ("mta assert nodep")
      MTASTREAMS ()
      for (size_t ek = 0; ek < eblen; ++ek) {
        if (!stinger_eb_is_blank (eb, ek))
          stinger_int64_fetch_add (&nactive_edges, 1);
      }
  }
  STINGER_FORALL_EB_END ();

  return nactive_edges;
}

/**
* @brief Sorts a batch of edge insertions and deletions.
* 
* Takes an array of edge insertions and deletions and sorts them.  The array is
* packed with <source, destination> pairs, such that actions[2*i] = source vertex
* ID and actions[2*i+1] = destination vertex ID.  Bit-wise complement the source
* and destination vertex IDs to indicate a deletion.  This function will create
* an undirected actions list (i.e. create the reverse edge action).
*
* The output is similarly packed, sorted by source vertex ID first, then by
* destination vertex ID such that deletions are contiguous before insertions.
* For vertex i, deletions incident on i start at deloff[i] and end at insoff[i].
* Insertions incident on i start at insoff[i] and end at deloff[i+1].
*
* @param nactions Number of edge actions
* @param actions The packed array of edge insertions and deletions
* @param insoff For each incident vertex in the batch, the offset into act[] of
*   the first edge insertion
* @param deloff For each incident vertex in the batch, the offset into act[] of
*   the first edge deletion
* @param act Sorted array of edge insertions and deletions
*
* @return The number of incident vertices in the batch (the size of insoff[] and deloff[])
*/
int64_t
stinger_sort_actions (int64_t nactions, int64_t * actions,
                      int64_t * insoff, int64_t * deloff, int64_t * act)
{
  int64_t head = 0;
  int64_t actlen;
  int64_t n;
  int64_t *actk = xmalloc ((2 * 2 * nactions + 1) * sizeof (*actk));

  OMP("omp parallel") {
  /* Copy & make i positive if necessary.
     Negative j still implies deletion. */
  MTA ("mta assert nodep")
    MTA ("mta block schedule")
    OMP ("omp for")
    for (int64_t k = 0; k < nactions; k++) {
      const int64_t i = actions[2 * k];
      const int64_t j = actions[2 * k + 1];

      if (i != j) {
        int64_t where = stinger_int64_fetch_add (&head, 2);
        assert (where < 2 * nactions);

        act[2 * where] = (i < 0 ? -i - 1 : i);
        act[2 * where + 1] = j;
        act[2 * (where + 1)] = (j < 0 ? -j - 1 : j);
        act[2 * (where + 1) + 1] = i;
      }
    }

  OMP("omp single") {
  actlen = head;
#if !defined(__MTA__)
  qsort (act, actlen, 2 * sizeof (act[0]), i2cmp); 
  //radix_sort_pairs (act, actlen<<1, 6);
#else
  bucket_sort_pairs (act, actlen);
#endif

  actk[0] = 0;
  n = 1;
  }

  /* Find local indices... */
  MTA ("mta assert nodep")
    OMP ("omp for")
    for (int64_t k = 1; k < actlen; k++) {
      if (act[2 * k] != act[2 * (k - 1)]) {
        stinger_int64_fetch_add (&n, 1);
        actk[k] = 1;
      } else
        actk[k] = 0;
    }

  prefix_sum (actlen, actk);
  //XXX assert(actk[actlen-1] == n-1);

  MTA ("mta assert nodep")
    OMP ("omp for")
    for (int64_t k = 0; k <= n; k++)
      deloff[k] = 0;

  OMP ("omp for")
    MTA ("mta assert nodep")
    for (int64_t k = 0; k < actlen; k++)
      stinger_int64_fetch_add (&deloff[actk[k] + 1], 1);

  prefix_sum (n+1, deloff);
  assert (deloff[n] == actlen);

  MTA ("mta assert nodep")
    MTA ("mta loop norestructure")
    OMP ("omp for")
    for (int64_t k = 0; k < n; k++) {
      int off;
      const int endoff = deloff[k + 1];

      for (off = deloff[k]; off < endoff; off++)
        if (act[2 * off + 1] >= 0)
          break;
      insoff[k] = off;
      assert (insoff[k] == deloff[k + 1]
              || act[2 * insoff[k]] == act[2 * deloff[k]]);
      for (off = deloff[k]; off < insoff[k]; off++) {
        assert (act[2 * off + 1] < 0);
        act[2 * off + 1] = -act[2 * off + 1] - 1;
      }
    }
  }

  free (actk);

  return n;                     /* number of unique vertices, elements in insoff[], deloff[] */
}

/** @brief Removes all the edges in the graph of a given type.
 *
 *  Traverses all edge blocks of a particular type in parallel and erases all
 *  edges of the specified type.  Blocks are available immediately for reuse.
 *
 *  @param G The STINGER data structure
 *  @param type The edge type of edges to delete
 *  @return Void
 */
void
stinger_remove_all_edges_of_type (struct stinger *G, int64_t type)
{
  int64_t ne_removed = 0;
  MAP_STING(G);
  /* TODO fix bugs here */
  MTA("mta assert parallel")
  OMP("omp parallel for reduction(+: ne_removed)")
  for (uint64_t p = 0; p < ETA(G, type)->high; p++) {
    struct stinger_eb *current_eb = ebpool->ebpool + ETA(G,type)->blocks[p];
    int64_t thisVertex = current_eb->vertexID;
    int64_t high = current_eb->high;
    struct stinger_edge * edges = current_eb->edges;

    int64_t removed = 0;
    for (uint64_t i = 0; i < high; i++) {
      int64_t neighbor = edges[i].neighbor;
	if (neighbor >= 0) {
	removed++;
	assert(neighbor >= 0);
	stinger_indegree_increment_atomic(G, neighbor, -1);
	edges[i].neighbor = -1;
      }
    }
    stinger_outdegree_increment_atomic(G, thisVertex, -removed);
    ne_removed += removed;
    current_eb->high = 0;
    current_eb->numEdges = 0;
    current_eb->smallStamp = INT64_MAX;
    current_eb->largeStamp = INT64_MIN;
  }
}

/** @brief Removes a vertex and all incident edges from the graph
 *
 *  Removes all edges incident to the vertex and unmaps the vertex
 *  in the physmap.  This algorithm first removes all out edges. During
 *  this pass it will assume an undirected graph and attempt to remove
 *  all reverse edges if they exist.  In the case of an undirected graph
 *  the algorithm will be complete and will terminate quickly.
 *
 *  If, however, the graph is a directed graph and there are incident edges
 *  still pointing to the vertex (stinger_indegree_get for the vtx > 0).  Then
 *  all edges are traversed looking for incident edges until the indegree is 0.
 *
 *  As a final step, the vertex is removed from the physmap, freeing this vertex
 *  to be re-assigned to a new vertex string.
 *
 *  @param G The STINGER data structure
 *  @param vtx_id The vertex to remove
 *  @return 0 if deletion is successful, -1 if it fails
 */
int64_t
stinger_remove_vertex (struct stinger *G, int64_t vtx_id)
{
  // Remove out edges
  STINGER_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(G,vtx_id) {
    stinger_remove_edge_pair(G,STINGER_EDGE_TYPE,STINGER_EDGE_SOURCE,STINGER_EDGE_DEST);
  } STINGER_PARALLEL_FORALL_EDGES_OF_VTX_END();

  // Remove remaining in edges
  if (stinger_indegree_get(G,vtx_id) > 0) {
    STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(G) {
      int64_t from = STINGER_EDGE_SOURCE;
      int64_t to = STINGER_EDGE_DEST;
      int64_t type = STINGER_EDGE_TYPE;
      if (to == vtx_id) {
        stinger_remove_edge(G,type,from,to);
        if (stinger_indegree_get(G,vtx_id) == 0) {
          break;
        }
      }
    } STINGER_FORALL_EDGES_OF_ALL_TYPES_END();
  }

  // Remove from physmap
  return stinger_physmap_vtx_remove_id(stinger_physmap_get(G),stinger_vertices_get(G),vtx_id);
}

const int64_t endian_check = 0x1234ABCD;
/** @brief Checkpoint a STINGER data structure to disk.
 *  Format (64-bit words):
 *    - endianness check
 *    - max NV (max vertex ID + 1)
 *    - number of edge types
 *    - type offsets (length = etypes + 1) offsets of the beginning on each type in the CSR structure
 *    - offsets (length = etypes * (maxNV+2)) concatenated CSR offsets, one NV+2-array per edge type
 *    - ind/adj (legnth = type_offsets[etypes] = total number of edges) concatenated CSR adjacency arrays, 
 *		      offsets into this are type_offsets[etype] + offets[etype*(maxNV+2) + sourcevertex]
 *    - weight (legnth = type_offsets[etypes] = total number of edges) concatenated csr weight arrays, 
 *		      offsets into this are type_offsets[etype] + offets[etype*(maxnv+2) + sourcevertex]
 *    - time first (legnth = type_offsets[etypes] = total number of edges) concatenated csr timefirst arrays, 
 *		      offsets into this are type_offsets[etype] + offets[etype*(maxnv+2) + sourcevertex]
 *    - time recent (legnth = type_offsets[etypes] = total number of edges) concatenated csr time recent arrays, 
 *		      offsets into this are type_offsets[etype] + offets[etype*(maxnv+2) + sourcevertex]
 * @param S A pointer to the Stinger to be saved.
 * @param maxVtx The maximum vertex ID + 1.
 * @param stingerfile The path and name of the output file.
 * @return 0 on success, -1 on failure.
 */
int
stinger_save_to_file (struct stinger * S, uint64_t maxVtx, const char * stingerfile)
{
#define tdeg(X,Y) offsets[((X) * (maxVtx+2)) + (Y+2)]

  /* TODO TODO TODO fix this function and the load function to work wihtout static sizes */

  MAP_STING(S);
  uint64_t * restrict offsets = xcalloc((maxVtx+2) * (S->max_netypes), sizeof(uint64_t));

  for(int64_t type = 0; type < (S->max_netypes); type++) {
    struct stinger_eb * local_ebpool = ebpool->ebpool;
    OMP("omp parallel for")
    MTA("mta assert parallel")
    for(uint64_t block = 0; block < ETA(S,type)->high; block++) {
      struct stinger_eb * cureb = local_ebpool + ETA(S, type)->blocks[block];
      int64_t num = cureb->numEdges;
      if (num) {
	stinger_int64_fetch_add(&tdeg(type,cureb->vertexID), num);
      }
    }
  }

  for(int64_t type = 0; type < (S->max_netypes); type++) {
    prefix_sum(maxVtx+2, offsets + (type * (maxVtx+2)));
  }

  uint64_t * restrict type_offsets = xmalloc(((S->max_netypes)+1) * sizeof(uint64_t));

  type_offsets[0] = 0;
  for(int64_t type = 0; type < (S->max_netypes); type++) {
    type_offsets[type+1] = offsets[type * (maxVtx + 2) + (maxVtx + 1)] + type_offsets[type];
  }

  int64_t * restrict ind = xmalloc(4 * type_offsets[(S->max_netypes)] * sizeof(int64_t));
  int64_t * restrict weight = ind + type_offsets[(S->max_netypes)];
  int64_t * restrict timefirst = weight + type_offsets[(S->max_netypes)];
  int64_t * restrict timerecent = timefirst + type_offsets[(S->max_netypes)];

#undef tdeg
#define tdeg(X,Y) offsets[((X) * (maxVtx+2)) + (Y+1)]

  for(int64_t type = 0; type < (S->max_netypes); type++) {
    struct stinger_eb * local_ebpool = ebpool->ebpool;
    OMP("omp parallel for")
    MTA("mta assert parallel")
    for(uint64_t block = 0; block < ETA(S,type)->high; block++) {
      struct stinger_eb * cureb = local_ebpool + ETA(S,type)->blocks[block];
      int64_t num = cureb->numEdges;
      if (num) {
	int64_t my_off = stinger_int64_fetch_add(&tdeg(type,cureb->vertexID),num) + type_offsets[type];
	int64_t stop = my_off + num;
	for(uint64_t k = 0; k < stinger_eb_high(cureb) && my_off < stop; k++) {
	  if(!stinger_eb_is_blank(cureb, k)) {
	    ind[my_off] = stinger_eb_adjvtx(cureb,k);
	    weight[my_off] = stinger_eb_weight(cureb,k);
	    timefirst[my_off] = stinger_eb_first_ts(cureb,k);
	    timerecent[my_off] = stinger_eb_ts(cureb,k);
	    my_off++;
	  }
	}
      }
    }
  }
#undef tdeg

#if !defined(__MTA__)
  FILE * fp = fopen(stingerfile, "wb");

  if(!fp) {
    fprintf (stderr, "%s %d: Can't open file \"%s\" for writing\n", __func__, __LINE__, stingerfile);
    free(offsets); free(type_offsets); free(ind);
    return -1;
  }

  int64_t etypes = (S->max_netypes);
  int64_t written = 0;

  written += fwrite(&endian_check, sizeof(int64_t), 1, fp);
  written += fwrite(&maxVtx, sizeof(int64_t), 1, fp);
  written += fwrite(&etypes, sizeof(int64_t), 1, fp);
  written += fwrite(type_offsets, sizeof(int64_t), etypes+1, fp);
  written += fwrite(offsets, sizeof(int64_t), (maxVtx+2) * etypes, fp);
  written += fwrite(ind, sizeof(int64_t), 4 * type_offsets[etypes], fp);

  for(uint64_t v = 0; v < maxVtx; v++) {
    int64_t vdata[2];
    vdata[0] = stinger_vtype_get(S,v);
    vdata[1] = stinger_vweight_get(S,v);
    fwrite(vdata, sizeof(int64_t), 2, fp);
  }

  stinger_names_save(stinger_physmap_get(S), fp);
  stinger_names_save(stinger_vtype_names_get(S), fp);
  stinger_names_save(stinger_etype_names_get(S), fp);

  fclose(fp);
#else
  xmt_luc_io_init();
  int64_t etypes = (S->max_netypes);
  int64_t written = 0;
  size_t sz = (3 + etypes + 1 + (maxVtx+2) * etypes + 4 * type_offsets[etypes]) * sizeof(int64_t);
  int64_t * xmt_buf = xmalloc (sz);
  xmt_buf[0] = endian_check;
  xmt_buf[1] = maxVtx;
  xmt_buf[2] = etypes;
  for (int64_t i = 0; i < etypes+1; i++) {
    xmt_buf[3+i] = type_offsets[i];
  }
  for (int64_t i = 0; i < (maxVtx+2) * etypes; i++) {
    xmt_buf[3+etypes+1+i] = offsets[i];
  }
  for (int64_t i = 0; i < 4 * type_offsets[etypes]; i++) {
    xmt_buf[3+etypes+1+(maxVtx+2)*etypes+i] = ind[i];
  }
  xmt_luc_snapout(stingerfile, xmt_buf, sz);
  written = sz;
  free(xmt_buf);
#endif

  if(written != (3 + etypes + 1 + (maxVtx+2) * etypes + 4 * type_offsets[etypes]) * sizeof(int64_t)) {
    free(offsets); free(type_offsets); free(ind);
    return -1;
  } else {
    free(offsets); free(type_offsets); free(ind);
    return 0;
  }
}

/** @brief Restores a STINGER checkpoint from disk.
 * @param stingerfile The path and name of the input file.
 * @param S A pointer to an empty STINGER structure.
 * @param maxVtx Output pointer for the the maximum vertex ID + 1.
 * @return 0 on success, -1 on failure.
*/
int
stinger_open_from_file (const char * stingerfile, struct stinger * S, uint64_t * maxVtx)
{
#if !defined(__MTA__)
  FILE * fp = fopen(stingerfile, "rb");

  if(!fp) {
    fprintf (stderr, "%s %d: Can't open file \"%s\" for reading\n", __func__, __LINE__, stingerfile);
    return -1;
  }
#endif

  if (!S) {
    return -1;
  }

  int64_t local_endian;
  int64_t etypes = 0;
#if !defined(__MTA__)
  int result = 0;
  result += fread(&local_endian, sizeof(int64_t), 1, fp);
  result += fread(maxVtx, sizeof(int64_t), 1, fp);
  result += fread(&etypes, sizeof(int64_t), 1, fp);
  
  if(result != 3) {
    fprintf (stderr, "%s %d: Fread of file \"%s\" failed.\n", __func__, __LINE__, stingerfile);
    return -1;
  }
#else
  xmt_luc_io_init();
  size_t sz;
  xmt_luc_stat(stingerfile, &sz);
  int64_t * xmt_buf = (int64_t *) xmalloc (sz);
  xmt_luc_snapin(stingerfile, xmt_buf, sz);
  local_endian = xmt_buf[0];
  *maxVtx = xmt_buf[1];
  etypes = xmt_buf[2];
#endif

  if(local_endian != endian_check) {
    *maxVtx = bs64(*maxVtx);
    etypes = bs64(etypes);
  }

  if(*maxVtx > (S->max_nv)) {
    fprintf (stderr, "%s %d: Vertices in file \"%s\" larger than the maximum number of vertices.\n", __func__, __LINE__, stingerfile);
    return -1;
  }
  if(etypes > (S->max_netypes)) {
    fprintf (stderr, "%s %d: Edge types in file \"%s\" larger than the maximum number of edge types.\n", __func__, __LINE__, stingerfile);
    return -1;
  }

#if !defined(__MTA__)
  uint64_t * restrict offsets = xcalloc((*maxVtx+2) * etypes, sizeof(uint64_t));
  uint64_t * restrict type_offsets = xmalloc((etypes+1) * sizeof(uint64_t));

  result = fread(type_offsets, sizeof(int64_t), etypes+1, fp);
  result += fread(offsets, sizeof(int64_t), (*maxVtx+2) * etypes, fp);

  if(result != etypes + 1 + (((*maxVtx) + 2) * etypes)) {
    fprintf (stderr, "%s %d: Fread of file \"%s\" failed. Types and type offsets failed.\n", __func__, __LINE__, stingerfile);
    return -1;
  }
#else
  uint64_t * restrict type_offsets = &xmt_buf[3];
  uint64_t * restrict offsets = type_offsets + etypes+1;
#endif

  if(local_endian != endian_check) {
    bs64_n(etypes + 1, type_offsets);
    bs64_n(*maxVtx + 2, offsets);
  }

#if !defined(__MTA__)
  int64_t * restrict ind = xmalloc(4 * type_offsets[etypes] * sizeof(int64_t));
  int64_t * restrict weight = ind + type_offsets[etypes];
  int64_t * restrict timefirst = weight + type_offsets[etypes];
  int64_t * restrict timerecent = timefirst + type_offsets[etypes];
  result = fread(ind, sizeof(int64_t), 4 * type_offsets[etypes], fp);

  if(result != 4 * type_offsets[etypes]) {
    fprintf (stderr, "%s %d: Fread of file \"%s\" failed. Edges and types failed.\n", __func__, __LINE__, stingerfile);
    return -1;
  }
#else
  int64_t * restrict ind = offsets + (*maxVtx+2)*etypes;
  int64_t * restrict weight = ind + type_offsets[etypes];
  int64_t * restrict timefirst = weight + type_offsets[etypes];
  int64_t * restrict timerecent = timefirst + type_offsets[etypes];
#endif

  if(local_endian != endian_check) {
    bs64_n(4 * type_offsets[etypes], ind);
  }

  for(int64_t type = 0; type < etypes; type++) {
    stinger_set_initial_edges(S, *maxVtx, type, 
	offsets + type * (*maxVtx+2),
	ind + type_offsets[type],
	weight + type_offsets[type],
	timerecent + type_offsets[type],
	timefirst + type_offsets[type],
	0);
  }

  int ignore = 0;
  for(uint64_t v = 0; v < *maxVtx; v++) {
    int64_t vdata[2];
    ignore = fread(vdata, sizeof(int64_t), 2, fp);
    stinger_vtype_set(S, v, vdata[0]);
    stinger_vweight_set(S, v, vdata[1]);
  }

  stinger_names_load(stinger_physmap_get(S), fp);
  stinger_names_load(stinger_vtype_names_get(S), fp);
  stinger_names_load(stinger_etype_names_get(S), fp);

#if !defined(__MTA__)
  fclose(fp);
  free(offsets); free(type_offsets); free(ind);
#else
  free(xmt_buf);
#endif
  return 0;
}

#if defined(STINGER_TEST_SAVE_LOAD)

int
main(int argc, char *argv[]) 
{
  stinger_t * S = stinger_new();

  for(int64_t i = 0; i < (S->max_netypes); i++) {
    char name[128];
    sprintf(name, "%ld", (long)i);

    int64_t out = 0;
    stinger_etype_names_create_type(S, name, &out);

    if(i != out)
      LOG_E("Etype doesn't have expected number");
  }

  for(int64_t i = 0; i < (S->max_nvtypes); i++) {
    char name[128];
    sprintf(name, "%ld", (long)i);

    int64_t out = 0;
    stinger_vtype_names_create_type(S, name, &out);

    if(i != out)
      LOG_E("Vtype doesn't have expected number");
  }

  for(int64_t i = 0; i < 1024; i++) {
    char name[128];
    sprintf(name, "%ld", (long)i);

    int64_t out = 0;
    stinger_mapping_create(S, name, strlen(name), &out);

    if(i != out)
      LOG_E("Vertex doesn't have expected number");
  }

  for(int64_t i = 0; i < 1024; i++) {
    stinger_insert_edge(S, 0, i, i+1, 1, i);
  }

  stinger_save_to_file(S, stinger_max_active_vertex(S) + 1, "test_stinger.bin");

  stinger_free_all(S);

  int64_t nv;
  stinger_open_from_file("test_stinger.bin", &S, &nv);

  for(int64_t i = 0; i < (S->max_netypes); i++) {
    char name[128];
    sprintf(name, "%ld", (long)i);

    int64_t out = 0;
    char * name_out = stinger_etype_names_lookup_name(S, i);

    if(0 != strcmp(name_out, name))
      LOG_E_A("Etype doesn't have expected number <%s vs %s>", name_out, name);

    if(i != stinger_etype_names_lookup_type(S, name))
      LOG_E("etype doesn't have expected number");
  }

  for(int64_t i = 0; i < (S->max_nvtypes); i++) {
    char name[128];
    sprintf(name, "%ld", (long)i);

    int64_t out = 0;
    char * name_out = stinger_vtype_names_lookup_name(S, i);

    if(0 != strcmp(name_out, name))
      LOG_E("vtype doesn't have expected number");

    if(i != stinger_vtype_names_lookup_type(S, name))
      LOG_E("Vtype doesn't have expected number");
  }

  for(int64_t i = 0; i < 1024; i++) {
    char name[128];
    sprintf(name, "%ld", (long)i);

    int64_t out = 0;
    int64_t len;
    char * name_out;
    stinger_mapping_physid_direct(S, i, &name_out, &len);

    if(0 != strncmp(name, name_out, len))
      LOG_E("Vertex doesn't have expected number");
  }

  for(int64_t i = 0; i < 1024; i++) {
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
      if(STINGER_EDGE_DEST != i + 1) 
	LOG_E("Vertex doesn't have expected number");
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }
}

#endif

#if !defined(STINGER_H_)
#define STINGER_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include <stdint.h>

#include "stinger_vertex.h"
#include "stinger_names.h"
#include "stinger_physmap.h"
#include "stinger_defs.h"

/* User-accessible data structures */
struct stinger_edge;
typedef struct stinger stinger_t;

/* STINGER creation & deletion */
struct stinger *stinger_new (void);

struct stinger *stinger_new_full (int64_t nv, int64_t nebs, int64_t netypes, int64_t nvtypes);

void stinger_set_initial_edges (struct stinger * /* G */ ,
				const size_t     /* nv */ ,
				const int64_t    /* EType */ ,
				const int64_t *  /* off */ ,
				const int64_t *  /* phys_to */ ,
				const int64_t *  /* weights */ ,
				const int64_t *  /* times */ ,
				const int64_t *  /* first_times */ ,
				const int64_t    /* single_ts, if !times or !first_times */);

struct stinger *stinger_free (struct stinger *);

struct stinger *stinger_free_all (struct stinger *);

vindex_t stinger_max_nv(stinger_t * S);

int64_t stinger_max_num_etypes(stinger_t * S);

/* read and write stinger from disk 
 * writes stinger into series of files in the specified directory
 * reading back will allocate a new STINGER
 */
int stinger_save_to_file (struct stinger * /* STINGER */, 
			  uint64_t /* max active vtx */, 
			  const char * /* directory name */);

int stinger_open_from_file (const char * /* directory */, 
			    struct stinger ** /* reference for empty output pointer */, 
			    uint64_t * /* reference for output max vtx */);

/* Edge insertion and deletion */
int stinger_insert_edge (struct stinger *, int64_t /* type */ ,
			 int64_t /* from */ , int64_t /* to */ ,
			 int64_t /* int weight */ ,
			 int64_t /* timestamp */ );

int stinger_insert_edge_pair (struct stinger *, int64_t /* type */ ,
			      int64_t /* from */ , int64_t /* to */ ,
			      int64_t /* int weight */ ,
			      int64_t /* timestamp */ );

int stinger_incr_edge (struct stinger *, int64_t /* type */ ,
		       int64_t /* from */ , int64_t /* to */ ,
		       int64_t /* int weight */ ,
		       int64_t /* timestamp */ );

int stinger_incr_edge_pair (struct stinger *, int64_t /* type */ ,
			    int64_t /* from */ , int64_t /* to */ ,
			    int64_t /* int weight */ ,
			    int64_t /* timestamp */ );

int stinger_remove_edge (struct stinger *, int64_t /* type */ ,
			 int64_t /* from */ , int64_t /* to */ );

int stinger_remove_edge_pair (struct stinger *, int64_t /* type */ ,
			      int64_t /* from */ , int64_t /* to */ );

void stinger_remove_all_edges_of_type (struct stinger *G, int64_t type);

/* Edge metadata (directed)*/
int64_t stinger_edgeweight (const struct stinger *, int64_t /* vtx 1 */ ,
			    int64_t /* vtx 2 */ ,
			    int64_t /* type */ );

int stinger_set_edgeweight (struct stinger *, int64_t /* vtx 1 */ ,
			    int64_t /* vtx 2 */ ,
			    int64_t /* type */ ,
			    int64_t /*weight */ );

int64_t stinger_edge_timestamp_first (const struct stinger *,
				      int64_t /* vtx 1 */ ,
				      int64_t /* vtx 2 */ ,
				      int64_t /* type */ );

int64_t stinger_edge_timestamp_recent (const struct stinger *,
				       int64_t /* vtx 1 */ ,
				       int64_t /* vtx 2 */ ,
				       int64_t /* type */ );

int stinger_edge_touch (struct stinger *, int64_t /* vtx 1 */ ,
			int64_t /* vtx 2 */ ,
			int64_t /* type */ ,
			int64_t /* timestamp */ );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ * 
 * INTERNAL "Objects"
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
stinger_vertices_t *
stinger_vertices_get(const stinger_t * S);

stinger_physmap_t *
stinger_physmap_get(const stinger_t * S);

stinger_names_t *
stinger_vtype_names_get(const stinger_t * S);

stinger_names_t *
stinger_etype_names_get(const stinger_t * S);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ * 
 * VERTEX METADATA
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* IN DEGREE */

vdegree_t
stinger_indegree_get(const stinger_t * S, vindex_t v);

vdegree_t
stinger_indegree_set(const stinger_t * S, vindex_t v, vdegree_t d);

vdegree_t
stinger_indegree_increment(const stinger_t * S, vindex_t v, vdegree_t d);

vdegree_t
stinger_indegree_increment_atomic(const stinger_t * S, vindex_t v, vdegree_t d);

/* OUT DEGREE */

vdegree_t
stinger_outdegree_get(const stinger_t * S, vindex_t v);

vdegree_t
stinger_outdegree_set(const stinger_t * S, vindex_t v, vdegree_t d);

vdegree_t
stinger_outdegree_increment(const stinger_t * S, vindex_t v, vdegree_t d);

vdegree_t
stinger_outdegree_increment_atomic(const stinger_t * S, vindex_t v, vdegree_t d);

/* TYPE */

vtype_t
stinger_vtype_get(const stinger_t * S, vindex_t v);

vtype_t
stinger_vtype_set(const stinger_t * S, vindex_t v, vtype_t type);

/* WEIGHT */
vweight_t
stinger_vweight_get(const stinger_t * S, vindex_t v);

vweight_t
stinger_vweight_set(const stinger_t * S, vindex_t v, vweight_t weight);

vweight_t
stinger_vweight_increment(const stinger_t * S, vindex_t v, vweight_t weight);

vweight_t
stinger_vweight_increment_atomic(const stinger_t * S, vindex_t v, vweight_t weight);

/* ADJACENCY */
adjacency_t
stinger_adjacency_get(const stinger_t * S, vindex_t v);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * PHYSICAL MAPPING
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int
stinger_mapping_create(const stinger_t * S, const char * byte_string, uint64_t length, int64_t * vtx_out);

vindex_t
stinger_mapping_lookup(const stinger_t * S, const char * byte_string, uint64_t length);

int
stinger_mapping_physid_get(const stinger_t * S, vindex_t vertexID, char ** outbuffer, uint64_t * outbufferlength);

int
stinger_mapping_physid_direct(const stinger_t * S, vindex_t vertexID, char ** out_ptr, uint64_t * out_len);

vindex_t
stinger_mapping_nv(const stinger_t * S);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * VTYPE NAMES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int
stinger_vtype_names_create_type(const stinger_t * S, const char * name, int64_t * out);

int64_t
stinger_vtype_names_lookup_type(const stinger_t * S, const char * name);

char *
stinger_vtype_names_lookup_name(const stinger_t * S, int64_t type);

int64_t
stinger_vtype_names_count(const stinger_t * S);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * ETYPE NAMES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int
stinger_etype_names_create_type(stinger_t * S, const char * name, int64_t * out);

int64_t
stinger_etype_names_lookup_type(const stinger_t * S, const char * name);

char *
stinger_etype_names_lookup_name(const stinger_t * S, int64_t type);

int64_t
stinger_etype_names_count(const stinger_t * S);

uint64_t stinger_max_active_vertex(const struct stinger * S);

uint64_t stinger_num_active_vertices(const struct stinger * S);

int64_t stinger_typed_outdegree(const struct stinger *, int64_t /* vtx */, int64_t /* type */);
XMTI int64_t stinger_indegree (const struct stinger *, int64_t);
XMTI int64_t stinger_vweight (const struct stinger *, int64_t);

int64_t stinger_set_vweight (struct stinger *, int64_t /* vertex */ , int64_t /* weight */ );


/* Neighbor list access */
void stinger_gather_successors (const struct stinger *,
                                          int64_t /* vtx */,
                                          size_t * /* outlen */,
                                          int64_t * /* out_vtx */,
					  int64_t * /* out_weight_optional */,
					  int64_t * /* out_timefirst_optional */,
                                          int64_t * /* out_timerecent_optional */,
					  int64_t * /* out_type_optional */,
                                          size_t max_outlen);

void stinger_gather_typed_successors (const struct stinger *,
				      int64_t /* type */ ,
				      int64_t /* vtx */ ,
				      size_t * /* outlen */ ,
				      int64_t * /* out */ ,
				      size_t /* max_outlen */ );

int stinger_has_typed_successor (const struct stinger *,
				 int64_t /* type */ , int64_t /* from */ ,
				 int64_t /* to */ );

void stinger_gather_typed_predecessors (const struct stinger *,
					int64_t /* type */,
				        int64_t /* v */,
				        size_t * /* outlen */,
				        int64_t * /* out */,
				        size_t /* max_outlen */ );

int64_t stinger_sort_actions (int64_t nactions,
			      int64_t * /* actions */,
			      int64_t * /* insoff */,
			      int64_t * /* deloff */,
			      int64_t * /* act */ );

/* Global Queries */
int64_t stinger_total_edges (const struct stinger *);

int64_t stinger_edges_up_to(const struct stinger * S, int64_t nv);

int64_t stinger_max_total_edges (const struct stinger * S);

size_t stinger_graph_size (const struct stinger *);

size_t
stinger_ebpool_size(int64_t nebs);

size_t
stinger_etype_array_size(int64_t nebs);

/* Graph traversal macros *
 * MUST BE CALLED IN PAIRS.
 * Alternatively, you may call copy out functions such as 
 * stinger_gather_typed_successors() or stinger_to_unsorted/sorted_csr()
 */

/* modify traversal macros *
 * These are UNSAFE WHEN OTHER THREADS OR PROCESSES MAY BE MODIFYING the graph
 * in that you may receive data in an inconsistent state.  If you modify
 * the destination, weight, first timestamp, or recent timestamp through these
 * it will affect the graph structure.  Source and edge type are local copies
 * that will not be stored.
 */
#define STINGER_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_FORALL_EDGES_OF_VTX_END() } while (0)

#define STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_END() while (0)

#define STINGER_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_PARALLEL_FORALL_EDGES_OF_VTX_END() while (0)

#define STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END() while (0)

#define STINGER_FORALL_EDGES_BEGIN(STINGER_,TYPE_) do {
#define STINGER_FORALL_EDGES_END() while (0)

#define STINGER_PARALLEL_FORALL_EDGES_BEGIN(STINGER_,TYPE_) do {
#define STINGER_PARALLEL_FORALL_EDGES_END() while (0)

/* read-only traversal macros *
 * These should be safe even when the graph is being modified elsewhere. All 
 * variables are local and will not be stored back in the graph.
 */
#define STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_END() while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END() while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_READ_ONLY_PARALLEL__FORALL_EDGES_OF_TYPE_OF_VTX_END() while (0)

#define STINGER_READ_ONLY_FORALL_EDGES_BEGIN(STINGER_,TYPE_) do {
#define STINGER_READ_ONLY_FORALL_EDGES_END() while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_BEGIN(STINGER_,TYPE_) do {
#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_END() while (0)

/* Use these to access the current edge inside the above macros */
#define STINGER_EDGE_SOURCE /* always read-only */
#define STINGER_EDGE_TYPE /* always read-only */
#define STINGER_EDGE_DEST /* do not set < 0 */
#define STINGER_EDGE_WEIGHT
#define STINGER_EDGE_TIME_FIRST
#define STINGER_EDGE_TIME_RECENT

/* Filtering traversal macros *
 * This macro will traverse all edges according to the specified filter.
 * For example, this code will add the weights of any edges 
 * from 4, 5, and 6 that are of type 1 or 2. 
 * uint64_t vertices[] = {4, 5, 6};
 * uint64_t etypes[] = {1, 2};
 * uint64_t weight;
 * STINGER_TRAVERSE_EDGES(S, OF_VERTICES(vertices, 3) OF_TYPE(etypes, 2),
 *    weight += STINGER_EDGE_WEIGHT;
 * )
 */
#define STINGER_TRAVERSE_EDGES(STINGER_, FILTER_, CODE_)
#define OF_VERTICES(ARRAY_, COUNT_)
#define OF_VERTEX_TYPE(ARRAY_, COUNT_)
#define OF_EDGE_TYPE(ARRAY_, COUNT_)
#define MODIFIED_BEFORE(TIME_)
#define MODIFIED_AFTER(TIME_)
#define CREATED_BEFORE(TIME_)
#define CREATED_AFTER(TIME_)
/* coming soon - parallel */

/* Basic test for structural problems - should return 0*/
uint32_t stinger_consistency_check (struct stinger *S, uint64_t NV);

#include "stinger_traversal.h"

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* STINGER_H_ */

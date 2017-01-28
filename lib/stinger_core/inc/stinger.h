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

#define EDGE_WEIGHT_SET 0x1
#define EDGE_WEIGHT_INCR 0x2

/* User-accessible data structures */
struct stinger_edge;
typedef struct stinger stinger_t;

#include "stinger_internal.h"

/* STINGER configuration structure */
struct stinger_config_t {
	int64_t nv;
	int64_t nebs;
	int64_t netypes;
	int64_t nvtypes;
	size_t memory_size;
	uint8_t no_map_none_etype;
	uint8_t no_map_none_vtype;
	uint8_t no_resize;
};

/* STINGER creation & deletion */
struct stinger *stinger_new (void);

struct stinger *stinger_new_full (struct stinger_config_t * config);

void stinger_set_initial_edges (struct stinger * /* G */ ,
				const size_t     /* nv */ ,
				const int64_t    /* EType */ ,
				const int64_t *  /* off */ ,
				const int64_t *  /* phys_to */ ,
				const int64_t *  /* direction */ ,
				const int64_t *  /* weights */ ,
				const int64_t *  /* times */ ,
				const int64_t *  /* first_times */ ,
				const int64_t    /* single_ts, if !times or !first_times */);

struct stinger *stinger_free (struct stinger *);

struct stinger *stinger_free_all (struct stinger *);

vindex_t stinger_max_nv(const stinger_t * S);

int64_t stinger_max_num_etypes(const stinger_t * S);

struct stinger_size_t calculate_stinger_size(int64_t nv, int64_t nebs, int64_t netypes, int64_t nvtypes);

/* read and write stinger from disk 
 * writes stinger into series of files in the specified directory
 * reading back will allocate a new STINGER
 */
int stinger_save_to_file (struct stinger * /* STINGER */, 
			  uint64_t /* max active vtx */, 
			  const char * /* directory name */);

int stinger_open_from_file (const char * /* directory */, 
			    struct stinger * /* reference for empty STINGER */, 
			    uint64_t * /* reference for output max vtx */);

/* Edge insertion and deletion */
int
stinger_update_directed_edge(struct stinger *G,
                     int64_t type, int64_t from, int64_t to,
                     int64_t weight, int64_t timestamp, int64_t direction,
                     int64_t operation);

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

int64_t stinger_remove_vertex(struct stinger *G, int64_t vtx_id);

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
stinger_vertices_get(stinger_t * S);

stinger_physmap_t *
stinger_physmap_get(stinger_t * S);

stinger_names_t *
stinger_vtype_names_get(stinger_t * S);

stinger_names_t *
stinger_etype_names_get(stinger_t * S);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ * 
 * VERTEX METADATA
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* DEGREE */

vdegree_t
stinger_degree_get(const stinger_t * S, vindex_t v);

vdegree_t
stinger_degree_set(stinger_t * S, vindex_t v, vdegree_t d);

vdegree_t
stinger_degree_increment(stinger_t * S, vindex_t v, vdegree_t d);

vdegree_t
stinger_degree_increment_atomic(stinger_t * S, vindex_t v, vdegree_t d);

/* IN DEGREE */

vdegree_t
stinger_indegree_get(const stinger_t * S, vindex_t v);

vdegree_t
stinger_indegree_set(stinger_t * S, vindex_t v, vdegree_t d);

vdegree_t
stinger_indegree_increment(stinger_t * S, vindex_t v, vdegree_t d);

vdegree_t
stinger_indegree_increment_atomic(stinger_t * S, vindex_t v, vdegree_t d);

/* OUT DEGREE */

vdegree_t
stinger_outdegree_get(const stinger_t * S, vindex_t v);

vdegree_t
stinger_outdegree_set(stinger_t * S, vindex_t v, vdegree_t d);

vdegree_t
stinger_outdegree_increment(stinger_t * S, vindex_t v, vdegree_t d);

vdegree_t
stinger_outdegree_increment_atomic(stinger_t * S, vindex_t v, vdegree_t d);

/* TYPE */

vtype_t
stinger_vtype_get(const stinger_t * S, vindex_t v);

vtype_t
stinger_vtype_set(stinger_t * S, vindex_t v, vtype_t type);

/* WEIGHT */
vweight_t
stinger_vweight_get(const stinger_t * S, vindex_t v);

vweight_t
stinger_vweight_set(stinger_t * S, vindex_t v, vweight_t weight);

vweight_t
stinger_vweight_increment(stinger_t * S, vindex_t v, vweight_t weight);

vweight_t
stinger_vweight_increment_atomic(stinger_t * S, vindex_t v, vweight_t weight);

/* ADJACENCY */
adjacency_t
stinger_adjacency_get(const stinger_t * S, vindex_t v);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * PHYSICAL MAPPING
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int
stinger_mapping_create(stinger_t * S, const char * byte_string, uint64_t length, int64_t * vtx_out);

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
stinger_vtype_names_create_type(stinger_t * S, const char * name, int64_t * out);

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

int64_t stinger_typed_degree(const struct stinger *, int64_t /* vtx */, int64_t /* type */);

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

/* Neighbor list access */
void stinger_gather_predecessors (const struct stinger *,
                                          int64_t /* vtx */,
                                          size_t * /* outlen */,
                                          int64_t * /* out_vtx */,
					  int64_t * /* out_weight_optional */,
					  int64_t * /* out_timefirst_optional */,
                                          int64_t * /* out_timerecent_optional */,
					  int64_t * /* out_type_optional */,
                                          size_t max_outlen);

void stinger_gather_typed_predecessors (const struct stinger *,
					int64_t /* type */,
				        int64_t /* v */,
				        size_t * /* outlen */,
				        int64_t * /* out */,
				        size_t /* max_outlen */ );

int stinger_has_typed_predecessor (const struct stinger *,
				 int64_t /* type */ , int64_t /* from */ ,
				 int64_t /* to */ );

/* Neighbor list access */
void stinger_gather_neighbors (const struct stinger *,
                                          int64_t /* vtx */,
                                          size_t * /* outlen */,
                                          int64_t * /* out_vtx */,
					  int64_t * /* out_weight_optional */,
					  int64_t * /* out_timefirst_optional */,
                                          int64_t * /* out_timerecent_optional */,
					  int64_t * /* out_type_optional */,
                                          size_t max_outlen);

void stinger_gather_typed_neighbors (const struct stinger *,
				      int64_t /* type */ ,
				      int64_t /* vtx */ ,
				      size_t * /* outlen */ ,
				      int64_t * /* out */ ,
				      size_t /* max_outlen */ );

int stinger_has_typed_neighbor (const struct stinger *,
				 int64_t /* type */ , int64_t /* from */ ,
				 int64_t /* to */ );

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
#define STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_FORALL_OUT_EDGES_OF_VTX_END() } while (0)

#define STINGER_FORALL_IN_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_FORALL_IN_EDGES_OF_VTX_END() } while (0)

#define STINGER_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_FORALL_EDGES_OF_VTX_END() } while (0)

#define STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END() } while (0)

#define STINGER_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END() } while (0)

#define STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_END() } while (0)

#define STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_END() } while (0)

#define STINGER_PARALLEL_FORALL_IN_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_PARALLEL_FORALL_IN_EDGES_OF_VTX_END() } while (0)

#define STINGER_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_PARALLEL_FORALL_EDGES_OF_VTX_END() } while (0)

#define STINGER_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END() } while (0)

#define STINGER_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END() } while (0)

#define STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END() } while (0)

#define STINGER_FORALL_EDGES_BEGIN(STINGER_,TYPE_) do {
#define STINGER_FORALL_EDGES_END() } while (0)

#define STINGER_PARALLEL_FORALL_EDGES_BEGIN(STINGER_,TYPE_) do {
#define STINGER_PARALLEL_FORALL_EDGES_END() } while (0)

#define STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(STINGER_) do {
#define STINGER_FORALL_EDGES_OF_ALL_TYPES_END() } while (0)

#define STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_BEGIN(STINGER_) do {
#define STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_END() } while (0)

/* read-only traversal macros *
 * These should be safe even when the graph is being modified elsewhere. All 
 * variables are local and will not be stored back in the graph.
 */
#define STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_FORALL_IN_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_READ_ONLY_FORALL_IN_EDGES_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_READ_ONLY_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) do {
#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) do {
#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END() } while (0)

#define STINGER_READ_ONLY_FORALL_EDGES_BEGIN(STINGER_,TYPE_) do {
#define STINGER_READ_ONLY_FORALL_EDGES_END() } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_BEGIN(STINGER_,TYPE_) do {
#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_END() } while (0)

/* Use these to access the current edge inside the above macros */
#define STINGER_EDGE_SOURCE /* always read-only */
#define STINGER_EDGE_TYPE /* always read-only */
#define STINGER_EDGE_DEST /* do not set < 0 */
#define STINGER_EDGE_DIRECTION
#define STINGER_EDGE_WEIGHT
#define STINGER_EDGE_TIME_FIRST
#define STINGER_EDGE_TIME_RECENT
#define STINGER_IS_OUT_EDGE
#define STINGER_IS_IN_EDGE

#define STINGER_RO_EDGE_SOURCE
#define STINGER_RO_EDGE_TYPE
#define STINGER_RO_EDGE_DEST
#define STINGER_RO_EDGE_DIRECTION
#define STINGER_RO_EDGE_WEIGHT
#define STINGER_RO_EDGE_TIME_FIRST
#define STINGER_RO_EDGE_TIME_RECENT
#define STINGER_RO_IS_OUT_EDGE
#define STINGER_RO_IS_IN_EDGE

/* Basic test for structural problems - should return 0*/
uint32_t stinger_consistency_check (struct stinger *S, uint64_t NV);

#include "stinger_traversal.h"

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* STINGER_H_ */

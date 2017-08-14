#ifndef  STINGER_VERTEX_H
#define  STINGER_VERTEX_H

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#include "stinger_names.h"

typedef int64_t adjacency_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * TYPES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#if !defined(STINGER_VERTEX_CUSTOM_TYPE)
  typedef int64_t vtype_t;
  #define JSON_VTYPE(NAME,VAL) JSON_INT64(NAME,VAL)
  #define XML_ATTRIBUTE_VTYPE(NAME,VAL) XML_ATTRIBUTE_INT64(NAME,VAL)
#endif

#if !defined(STINGER_VERTEX_CUSTOM_WEIGHT)
  typedef int64_t vweight_t;
  #define JSON_VWEIGHT(NAME,VAL) JSON_INT64(NAME,VAL)
  #define XML_ATTRIBUTE_VWEIGHT(NAME,VAL) XML_ATTRIBUTE_INT64(NAME,VAL)
  #define stinger_vweight_fetch_add_atomic(P,VAL) stinger_int64_fetch_add(P,VAL)
  static inline vweight_t
  stinger_vweight_fetch_add(vweight_t * p, vweight_t val) {
    vweight_t out = *p;
    *p += val;
    return out;
  }
#endif

typedef int64_t vdegree_t;
#define stinger_vdegree_fetch_add_atomic(P,VAL) stinger_int64_fetch_add(P,VAL)
static inline vdegree_t
stinger_vdegree_fetch_add(vdegree_t * p, vdegree_t val) {
  vdegree_t out = *p;
  *p += val;
  return out;
}
typedef int64_t vindex_t;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * STRUCTURES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef struct stinger_vertex	stinger_vertex_t;
typedef struct stinger_vertices	stinger_vertices_t;


/**
* @brief A vertex block in STINGER
*/
struct stinger_vertex
{
  vtype_t     type;	  /**< Vertex type */
  vweight_t   weight;     /**< Vertex weight */
  vdegree_t   inDegree;   /**< In-degree of the vertex */
  vdegree_t   outDegree;  /**< Out-degree of the vertex */
  vdegree_t   degree; /**< Degree when counting both in an out edges */
  adjacency_t edges;	  /**< Reference to the adjacency structure for this vertex */
#if defined(STINGER_VERTEX_KEY_VALUE_STORE)
  key_value_store_t attributes;
#endif
};

/**
 * @brief The global container of all vertices
 *
 * For now, just an array, but for the sake of modularity, let's use this 
 * interface so that in the future we could store indices on vtypes or 
 * use alternate storage structures
 */
struct stinger_vertices
{
  int64_t	    max_vertices;
  stinger_vertex_t  vertices[0];
};


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * VERTEX FUNCTIONS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

vtype_t
stinger_vertex_type_get(const stinger_vertices_t * vertices, vindex_t v);

vtype_t
stinger_vertex_type_set(stinger_vertices_t * vertices, vindex_t v, vtype_t type);

vweight_t
stinger_vertex_weight_get(const stinger_vertices_t * vertices, vindex_t v);

vweight_t
stinger_vertex_weight_set(stinger_vertices_t * vertices, vindex_t v, vweight_t weight);

vweight_t
stinger_vertex_weight_increment(stinger_vertices_t * vertices, vindex_t v, vweight_t weight);

vweight_t
stinger_vertex_weight_increment_atomic(stinger_vertices_t * vertices, vindex_t v, vweight_t weight);

vdegree_t
stinger_vertex_degree_get(const stinger_vertices_t * vertices, vindex_t v);

vdegree_t
stinger_vertex_degree_set(stinger_vertices_t * vertices, vindex_t v, vdegree_t degree);

vdegree_t
stinger_vertex_degree_increment(stinger_vertices_t * vertices, vindex_t v, vdegree_t degree);

vdegree_t
stinger_vertex_degree_increment_atomic(stinger_vertices_t * vertices, vindex_t v, vdegree_t degree);

vdegree_t
stinger_vertex_indegree_get(const stinger_vertices_t * vertices, vindex_t v);

vdegree_t
stinger_vertex_indegree_set(stinger_vertices_t * vertices, vindex_t v, vdegree_t degree);

vdegree_t
stinger_vertex_indegree_increment(stinger_vertices_t * vertices, vindex_t v, vdegree_t degree);

vdegree_t
stinger_vertex_indegree_increment_atomic(stinger_vertices_t * vertices, vindex_t v, vdegree_t degree);

vdegree_t
stinger_vertex_outdegree_get(const stinger_vertices_t * vertices, vindex_t v);

vdegree_t
stinger_vertex_outdegree_set(stinger_vertices_t * vertices, vindex_t v, vdegree_t degree);

vdegree_t
stinger_vertex_outdegree_increment(stinger_vertices_t * vertices, vindex_t v, vdegree_t degree);

vdegree_t
stinger_vertex_outdegree_increment_atomic(stinger_vertices_t * vertices, vindex_t v, vdegree_t degree);

adjacency_t
stinger_vertex_edges_get(const stinger_vertices_t * vertices, vindex_t v);

adjacency_t *
stinger_vertex_edges_pointer_get(stinger_vertices_t * vertices, vindex_t v);

adjacency_t
stinger_vertex_edges_get_and_lock(stinger_vertices_t * vertices, vindex_t v);

adjacency_t
stinger_vertex_edges_set(stinger_vertices_t * vertices, vindex_t v, adjacency_t edges);


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * VERTICES FUNCTIONS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

stinger_vertices_t *
stinger_vertices_new(int64_t max_vertices);

void
stinger_vertices_init(stinger_vertices_t * S, int64_t max_vertices);

size_t
stinger_vertices_size(int64_t max_vertices);

void
stinger_vertices_free(stinger_vertices_t ** vertices);

stinger_vertex_t *
stinger_vertices_vertex_get(stinger_vertices_t * vertices, vindex_t v);

const stinger_vertex_t *
const_stinger_vertices_vertex_get(const stinger_vertices_t * vertices, vindex_t v);

int64_t 
stinger_vertices_max_vertices_get(const stinger_vertices_t * vertices);

int64_t 
stinger_vertices_size_bytes(const stinger_vertices_t * vertices);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif  /*STINGER_VERTEX_H*/

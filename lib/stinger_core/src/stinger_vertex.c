#include "stinger_vertex.h"
#include "stinger_atomics.h"
#include "x86_full_empty.h"

#include <stdlib.h>
#include <stdio.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * STINGER VERTICES
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

inline stinger_vertices_t *
stinger_vertices_new(int64_t max_vertices)
{
  stinger_vertices_t * rtn = xcalloc(1, sizeof(stinger_vertices_t) + max_vertices * sizeof(stinger_vertex_t));
  rtn->max_vertices = max_vertices;
  return rtn;
}

inline void
stinger_vertices_init(stinger_vertices_t * S, int64_t max_vertices)
{
  S->max_vertices = max_vertices;
  return;
}

inline size_t 
stinger_vertices_size(int64_t max_vertices)
{
  size_t rtn = sizeof(stinger_vertices_t) + max_vertices * sizeof(stinger_vertex_t);
  return rtn;
}

inline void
stinger_vertices_free(stinger_vertices_t ** vertices)
{
  if(*vertices)
    free(*vertices);
  *vertices = NULL;
}

inline stinger_vertex_t *
stinger_vertices_vertex_get(const stinger_vertices_t * vertices, vindex_t v)
{
  if (v >= vertices->max_vertices || v < 0) {
    return NULL;
  }
  return &(vertices->vertices[v]);
}

inline int64_t
stinger_vertices_max_vertices_get(const stinger_vertices_t * vertices)
{
  return (vertices->max_vertices);
}

inline int64_t
stinger_vertices_size_bytes(const stinger_vertices_t * vertices)
{
  return (sizeof(stinger_vertices_t) + stinger_vertices_max_vertices_get(vertices) * sizeof(stinger_vertex_t));
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * STINGER VERTEX
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define VTX(v) stinger_vertices_vertex_get(vertices, v)
 
/* TYPE */

inline vtype_t
stinger_vertex_type_get(const stinger_vertices_t * vertices, vindex_t v)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return VTX(v)->type;
}

inline vtype_t
stinger_vertex_type_set(const stinger_vertices_t * vertices, vindex_t v, vtype_t type)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return (VTX(v)->type = type);
}

/* WEIGHT */

inline vweight_t
stinger_vertex_weight_get(const stinger_vertices_t * vertices, vindex_t v)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return VTX(v)->weight;
}

inline vweight_t
stinger_vertex_weight_set(const stinger_vertices_t * vertices, vindex_t v, vweight_t weight)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return (VTX(v)->weight = weight);
}

inline vweight_t
stinger_vertex_weight_increment(const stinger_vertices_t * vertices, vindex_t v, vweight_t weight)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return stinger_vweight_fetch_add(&(VTX(v)->weight), weight);
}

inline vweight_t
stinger_vertex_weight_increment_atomic(const stinger_vertices_t * vertices, vindex_t v, vweight_t weight)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return stinger_vweight_fetch_add_atomic(&(VTX(v)->weight), weight);
}

/* DEGREE */

inline vdegree_t
stinger_vertex_degree_get(const stinger_vertices_t * vertices, vindex_t v)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return VTX(v)->degree;
}

inline vdegree_t
stinger_vertex_degree_set(const stinger_vertices_t * vertices, vindex_t v, vdegree_t degree)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return (VTX(v)->degree = degree);
}

inline vdegree_t
stinger_vertex_degree_increment(const stinger_vertices_t * vertices, vindex_t v, vdegree_t degree)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return (VTX(v)->degree += degree);
}

inline vdegree_t
stinger_vertex_degree_increment_atomic(const stinger_vertices_t * vertices, vindex_t v, vdegree_t degree)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return stinger_vdegree_fetch_add_atomic(&(VTX(v)->degree), degree);
}

/* INDEGREE */

inline vdegree_t
stinger_vertex_indegree_get(const stinger_vertices_t * vertices, vindex_t v)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return VTX(v)->inDegree;
}

inline vdegree_t
stinger_vertex_indegree_set(const stinger_vertices_t * vertices, vindex_t v, vdegree_t degree)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return (VTX(v)->inDegree = degree);
}

inline vdegree_t
stinger_vertex_indegree_increment(const stinger_vertices_t * vertices, vindex_t v, vdegree_t degree)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return (VTX(v)->inDegree += degree);
}

inline vdegree_t
stinger_vertex_indegree_increment_atomic(const stinger_vertices_t * vertices, vindex_t v, vdegree_t degree)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return stinger_vdegree_fetch_add_atomic(&(VTX(v)->inDegree), degree);
}

/* OUTDEGREE */

inline vdegree_t
stinger_vertex_outdegree_get(const stinger_vertices_t * vertices, vindex_t v)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return VTX(v)->outDegree;
}

inline vdegree_t
stinger_vertex_outdegree_set(const stinger_vertices_t * vertices, vindex_t v, vdegree_t degree)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return (VTX(v)->outDegree = degree);
}

inline vdegree_t
stinger_vertex_outdegree_increment(const stinger_vertices_t * vertices, vindex_t v, vdegree_t degree)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return (VTX(v)->outDegree += degree);
}

inline vdegree_t
stinger_vertex_outdegree_increment_atomic(const stinger_vertices_t * vertices, vindex_t v, vdegree_t degree)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return stinger_vdegree_fetch_add_atomic(&(VTX(v)->outDegree), degree);
}

/* EDGES */

inline adjacency_t
stinger_vertex_edges_get(const stinger_vertices_t * vertices, vindex_t v)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return readff(&(VTX(v)->edges));
}

inline adjacency_t *
stinger_vertex_edges_pointer_get(const stinger_vertices_t * vertices, vindex_t v)
{  
  if (v >= vertices->max_vertices || v < 0) {
    return NULL;
  }
  return &(VTX(v)->edges);
}

inline adjacency_t
stinger_vertex_edges_get_and_lock(const stinger_vertices_t * vertices, vindex_t v)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return readfe(&(VTX(v)->edges));
}

inline adjacency_t
stinger_vertex_edges_set(const stinger_vertices_t * vertices, vindex_t v, adjacency_t edges)
{
  if (v >= vertices->max_vertices || v < 0) {
    return -1;
  }
  return (VTX(v)->edges = edges);
}

#if defined(STINGER_VERTEX_TEST)
int main(int argc, char *argv[]) {
  stinger_vertices_t * vertices = stinger_vertices_new(3);
  stinger_vertex_type_set(vertices, 0, 1);
  stinger_vertex_weight_set(vertices, 0, 1);
  stinger_vertex_weight_increment(vertices, 0, 1);
#pragma omp parallel for
  for(uint64_t i = 0; i < 98; i++) {
    stinger_vertex_weight_increment_atomic(vertices, 0, 1);
  }
  stinger_vertex_indegree_set(vertices, 0, 1);
  stinger_vertex_indegree_increment(vertices, 0, 1);
#pragma omp parallel for
  for(uint64_t i = 0; i < 98; i++) {
    stinger_vertex_indegree_increment_atomic(vertices, 0, 1);
  }
  stinger_vertex_outdegree_set(vertices, 0, 1);
  stinger_vertex_outdegree_increment(vertices, 0, 1);
#pragma omp parallel for
  for(uint64_t i = 0; i < 98; i++) {
    stinger_vertex_outdegree_increment_atomic(vertices, 0, 1);
  }
}
#endif

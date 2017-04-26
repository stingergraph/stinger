#if !defined(STINGER_TRAVERSAL_H_)
#define STINGER_TRAVERSAL_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include "stinger_internal.h"

#undef STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN
#undef STINGER_FORALL_OUT_EDGES_OF_VTX_END

#undef STINGER_FORALL_IN_EDGES_OF_VTX_BEGIN
#undef STINGER_FORALL_IN_EDGES_OF_VTX_END

#undef STINGER_FORALL_EDGES_OF_VTX_BEGIN
#undef STINGER_FORALL_EDGES_OF_VTX_END

#undef STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_BEGIN
#undef STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_END

#undef STINGER_PARALLEL_FORALL_IN_EDGES_OF_VTX_BEGIN
#undef STINGER_PARALLEL_FORALL_IN_EDGES_OF_VTX_END

#undef STINGER_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN
#undef STINGER_PARALLEL_FORALL_EDGES_OF_VTX_END

#undef STINGER_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_FORALL_EDGES_BEGIN
#undef STINGER_FORALL_EDGES_END

#undef STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN
#undef STINGER_FORALL_EDGES_OF_ALL_TYPES_END

#undef STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_BEGIN
#undef STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_END

#undef STINGER_PARALLEL_FORALL_EDGES_BEGIN
#undef STINGER_PARALLEL_FORALL_EDGES_END

#undef STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_VTX_END

#undef STINGER_READ_ONLY_FORALL_IN_EDGES_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_FORALL_IN_EDGES_OF_VTX_END

#undef STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END

#undef STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_READ_ONLY_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_VTX_END

#undef STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_VTX_END

#undef STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END

#undef STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_READ_ONLY_FORALL_EDGES_BEGIN
#undef STINGER_READ_ONLY_FORALL_EDGES_END

#undef STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_BEGIN
#undef STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_END

/* Use these to access the current edge inside the above macros */
#undef STINGER_EDGE_SOURCE
#undef STINGER_EDGE_TYPE
#undef STINGER_EDGE_DIRECTION
#undef STINGER_EDGE_DEST
#undef STINGER_EDGE_WEIGHT
#undef STINGER_EDGE_TIME_FIRST
#undef STINGER_EDGE_TIME_RECENT
#undef STINGER_IS_OUT_EDGE
#undef STINGER_IS_IN_EDGE

#undef STINGER_RO_EDGE_SOURCE
#undef STINGER_RO_EDGE_TYPE
#undef STINGER_RO_EDGE_DEST
#undef STINGER_RO_EDGE_DIRECTION
#undef STINGER_RO_EDGE_WEIGHT
#undef STINGER_RO_EDGE_TIME_FIRST
#undef STINGER_RO_EDGE_TIME_RECENT
#undef STINGER_RO_IS_OUT_EDGE
#undef STINGER_RO_IS_IN_EDGE

// Generic macro for iterating over all edges of a vertex. Edges are writable.
#define STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,EDGE_FILTER_,EB_FILTER_,PARALLEL_)\
  do {                                                                                                    \
    MAP_STING(STINGER_);                                                                                  \
    struct stinger_eb * ebpool_priv = ebpool->ebpool;                                                     \
    struct stinger_eb *  current_eb__ = ebpool_priv + stinger_vertex_edges_get(vertices, VTX_);           \
    while(current_eb__ != ebpool_priv) {                                                                  \
      int64_t source__ = current_eb__->vertexID;                                                          \
      int64_t type__ = current_eb__->etype;                                                               \
      EB_FILTER_ {                                                                                        \
        PARALLEL_                                                                                         \
        for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) {                               \
          if(!stinger_eb_is_blank(current_eb__, i__)) {                                                   \
            struct stinger_edge * current_edge__ = current_eb__->edges + i__;                             \
            EDGE_FILTER_ {

#define STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()         \
            } /* end EDGE_FILTER_ */                      \
          } /* end if eb blank */                         \
        } /* end for edges in eb */                       \
      } /* end EB_FILTER_ */                              \
      current_eb__ = ebpool_priv + (current_eb__->next);  \
    } /* end while not last edge */                       \
  } while (0)

// For all edges of vertex
#define STINGER_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,,,)
#define STINGER_FORALL_EDGES_OF_VTX_END() \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()

// For all out-edges of vertex
#define STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_IS_OUT_EDGE),,)
#define STINGER_FORALL_OUT_EDGES_OF_VTX_END() \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()

// For all in-edges of vertex
#define STINGER_FORALL_IN_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_IS_IN_EDGE),,)
#define STINGER_FORALL_IN_EDGES_OF_VTX_END() \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()

// For all edges of vertex of a certain edge type
#define STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,,if (current_eb__->etype == TYPE_),)
#define STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_END() \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()

// For all out-edges of vertex of a certain edge type
#define STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_IS_OUT_EDGE),if (current_eb__->etype == TYPE_),)
#define STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END() \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()

// For all out-edges of vertex of a certain edge type
#define STINGER_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_IS_IN_EDGE),if (current_eb__->etype == TYPE_),)
#define STINGER_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END() \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()

#define STINGER_FORALL_ENABLE_PARALLEL_ \
  OMP("omp parallel for")               \
  

// For all edges of vertex, in parallel
#define STINGER_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,,,STINGER_FORALL_ENABLE_PARALLEL_)
#define STINGER_PARALLEL_FORALL_EDGES_OF_VTX_END() \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()

// For all out-edges of vertex, in parallel
#define STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_IS_OUT_EDGE),,STINGER_FORALL_ENABLE_PARALLEL_)
#define STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_END() \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()

// For all in-edges of vertex, in parallel
#define STINGER_PARALLEL_FORALL_IN_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_IS_IN_EDGE),,STINGER_FORALL_ENABLE_PARALLEL_)
#define STINGER_PARALLEL_FORALL_IN_EDGES_OF_VTX_END() \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()

// For all edges of vertex of a certain edge type, in parallel
#define STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,,if (current_eb__->etype == TYPE_),STINGER_FORALL_ENABLE_PARALLEL_)
#define STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END() \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()

// For all out-edges of vertex of a certain edge type, in parallel
#define STINGER_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_IS_OUT_EDGE),if (current_eb__->etype == TYPE_),STINGER_FORALL_ENABLE_PARALLEL_)
#define STINGER_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END() \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()

// For all out-edges of vertex of a certain edge type, in parallel
#define STINGER_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_IS_IN_EDGE),if (current_eb__->etype == TYPE_),STINGER_FORALL_ENABLE_PARALLEL_)
#define STINGER_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END() \
  STINGER_GENERIC_FORALL_EDGES_OF_VTX_END()

// Generic macro for iterating over all edges. Edges are writable.
#define STINGER_GENERIC_FORALL_EDGES_BEGIN(STINGER_,SELECT_TYPE_,PARALLEL_) \
  do {                                                                                        \
    MAP_STING(STINGER_);                                                                      \
    SELECT_TYPE_ { /* This sets t__ */                                                        \
      struct stinger_eb * ebpool_priv = ebpool->ebpool;                                       \
      PARALLEL_                                                                               \
      for(uint64_t p__ = 0; p__ < ETA((STINGER_),(t__))->high; p__++) {                       \
        struct stinger_eb *  current_eb__ = ebpool_priv+ ETA((STINGER_),(t__))->blocks[p__];  \
        int64_t source__ = current_eb__->vertexID;                                            \
        int64_t type__ = current_eb__->etype;                                                 \
        for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) {                   \
          if(!stinger_eb_is_blank(current_eb__, i__)) {                                       \
            struct stinger_edge * current_edge__ = current_eb__->edges + i__;                 \
            if (STINGER_IS_OUT_EDGE) {

#define STINGER_GENERIC_FORALL_EDGES_END()  \
            } /* end if is out edge */      \
          } /* end if eb is blank */        \
        } /* end for edges in eb */         \
      } /* for each edge of type t__ */     \
    } /* END_SELECT_TYPE */                 \
  } while (0)

// For all edges of a given type
#define STINGER_FORALL_EDGES_BEGIN(STINGER_,TYPE_) \
  STINGER_GENERIC_FORALL_EDGES_BEGIN(STINGER_, uint64_t t__ = TYPE_;,)
#define STINGER_FORALL_EDGES_END() \
  STINGER_GENERIC_FORALL_EDGES_END()

// For all edges
#define STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(STINGER_) \
  STINGER_GENERIC_FORALL_EDGES_BEGIN(STINGER_,for (uint64_t t__ = 0; t__ < stinger_max_num_etypes(STINGER_); t__++),)
#define STINGER_FORALL_EDGES_OF_ALL_TYPES_END() \
  STINGER_GENERIC_FORALL_EDGES_END()

// For all edges of a given type, in parallel
#define STINGER_PARALLEL_FORALL_EDGES_BEGIN(STINGER_,TYPE_) \
  STINGER_GENERIC_FORALL_EDGES_BEGIN(STINGER_, uint64_t t__ = TYPE_;,STINGER_FORALL_ENABLE_PARALLEL_)
#define STINGER_PARALLEL_FORALL_EDGES_END() \
  STINGER_GENERIC_FORALL_EDGES_END()

// For all edges, in parallel
#define STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_BEGIN(STINGER_) \
  STINGER_GENERIC_FORALL_EDGES_BEGIN(STINGER_,for (uint64_t t__ = 0; t__ < stinger_max_num_etypes(STINGER_); t__++),STINGER_FORALL_ENABLE_PARALLEL_)
#define STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_END() \
  STINGER_GENERIC_FORALL_EDGES_END()

// Generic macro for iterating over all edges of a vertex. Edges are read-only.
#define STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,EDGE_FILTER_,EB_FILTER_) \
  do {                                                                                  \
    CONST_MAP_STING(STINGER_);                                                          \
    const struct stinger * restrict S__ = (STINGER_);                                   \
    const struct stinger_eb * restrict ebp__ = ebpool->ebpool;                          \
    const int64_t source__ = (VTX_);                                                    \
    int64_t ebp_k__ = vertices->vertices[source__].edges;                               \
    while(ebp_k__) {                                                                    \
      EB_FILTER_ {                                                                      \
        for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {                       \
          if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {                              \
            const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
            if(local_current_edge__.neighbor >= 0) {                                    \
              EDGE_FILTER_ {

#define STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_END() \
              } /* end EDGE_FILTER_ */              \
            } /* end if neighbor exists */          \
          } /* end if eb is blank */                \
        } /* end for all eb's */                    \
      } /* end EB_FILTER_ */                        \
      ebp_k__ = ebp__[ebp_k__].next;                \
    } /* end while ebp_k__ valid */                 \
  } while (0)

// For all edges of vertex
#define STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,,)
#define STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END() \
  STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_END()

// For all out-edges of vertex
#define STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_RO_IS_OUT_EDGE),)
#define STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_VTX_END() \
  STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_END()

// For all in-edges of vertex
#define STINGER_READ_ONLY_FORALL_IN_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_RO_IS_IN_EDGE),)
#define STINGER_READ_ONLY_FORALL_IN_EDGES_OF_VTX_END() \
  STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_END()

// For all edges of vertex of a certain edge type
#define STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,,if (ebp__[ebp_k__].etype == (TYPE_)))
#define STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_END() \
  STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_END()

// For all out-edges of vertex of a certain edge type
#define STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_RO_IS_OUT_EDGE),if (ebp__[ebp_k__].etype == (TYPE_)))
#define STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END() \
  STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_END()

// For all in-edges of vertex of a certain edge type
#define STINGER_READ_ONLY_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_RO_IS_IN_EDGE),if (ebp__[ebp_k__].etype == (TYPE_)))
#define STINGER_READ_ONLY_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END() \
  STINGER_GENERIC_READ_ONLY_FORALL_EDGES_OF_VTX_END()


// Generic macro for iterating over all edges of a vertex in parallel. Edges are read-only.
#define STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,EDGE_FILTER_,EB_FILTER_) \
  do {                                                                                      \
    CONST_MAP_STING(STINGER_);                                                              \
    const struct stinger * restrict S__ = (STINGER_);                                       \
    const struct stinger_eb * restrict ebp__ = ebpool->ebpool;                              \
    const int64_t source__ = (VTX_);                                                        \
    OMP("omp parallel") {                                                                   \
      OMP("omp single") {                                                                   \
        int64_t ebp_k__ = vertices->vertices[source__].edges;                               \
        while(ebp_k__) {                                                                    \
          EB_FILTER_ {                                                                      \
            OMP("omp task untied firstprivate(ebp_k__)")                                    \
            for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {                       \
              if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {                              \
                const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
                if(local_current_edge__.neighbor >= 0) {                                    \
                  EDGE_FILTER_ {

#define STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END() \
                  } /* end EDGE_FILTER_ */              \
                } /* end if neighbor exists */          \
              } /* end if eb is blank */                \
            } /* end for all eb's */                    \
          } /* end EB_FILTER_ */                        \
          ebp_k__ = ebp__[ebp_k__].next;                \
        } /* end while ebp_k__ valid */                 \
      } /* end omp single */                            \
    } OMP("omp taskwait"); /* end omp parallel */       \
  } while (0)

// For all edges of vertex, in parallel
#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,,)
#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END() \
  STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END()

// For all out-edges of vertex, in parallel
#define STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_RO_IS_OUT_EDGE),)
#define STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_VTX_END() \
  STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END()

// For all edges of vertex, in parallel
#define STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_RO_IS_IN_EDGE),)
#define STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_VTX_END() \
  STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END()

// For all edges of vertex of a certain edge type, in parallel
#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,,if (ebp__[ebp_k__].etype == (TYPE_)))
#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END()  \
  STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END()

// For all out-edges of vertex of a certain edge type, in parallel
#define STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_RO_IS_OUT_EDGE),if (ebp__[ebp_k__].etype == (TYPE_)))
#define STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END()  \
  STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END()

// For all in-edges of vertex of a certain edge type, in parallel
#define STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_,if (STINGER_RO_IS_IN_EDGE),if (ebp__[ebp_k__].etype == (TYPE_)))
#define STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END()  \
  STINGER_GENERIC_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END()

/* all edges of a given type */
#define STINGER_READ_ONLY_FORALL_EDGES_BEGIN(STINGER_,TYPE_)            \
      do {                                                              \
        CONST_MAP_STING(STINGER_); \
        const struct stinger * restrict S__ = (STINGER_);               \
        const struct stinger_eb * restrict ebp__ = ebpool->ebpool;	\
        const int64_t etype__ = (TYPE_);                                \
        for(uint64_t p__ = 0; p__ < ETA((STINGER_),(TYPE_))->high; p__++) {    \
          int64_t ebp_k__ = ETA((STINGER_),(TYPE_))->blocks[p__];              \
          const int64_t source__ = ebp__[ebp_k__].vertexID;             \
          const int64_t type__ = ebp__[ebp_k__].etype;                  \
          for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {     \
            if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {            \
              const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
              if(local_current_edge__.neighbor >= 0) {

#define STINGER_READ_ONLY_FORALL_EDGES_END()                            \
              }                                                         \
            }                                                           \
          }                                                             \
        }                                                               \
      } while (0)


#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_BEGIN(STINGER_,TYPE_)   \
      do {                                                              \
        CONST_MAP_STING(STINGER_); \
        const struct stinger * restrict S__ = (STINGER_);             \
        const int64_t etype__ = (TYPE_);                              \
        const struct stinger_eb * restrict ebp__ = ebpool->ebpool;	\
        OMP("omp parallel for ")                                            \
        for(uint64_t p__ = 0; p__ < ETA((STINGER_),(TYPE_))->high; p__++) { \
          int64_t ebp_k__ = ETA((STINGER_),(TYPE_))->blocks[p__];          \
          const int64_t source__ = ebp__[ebp_k__].vertexID;         \
          const int64_t type__ = ebp__[ebp_k__].etype;              \
          for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) { \
            if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {      \
              const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
              if(local_current_edge__.neighbor >= 0) {

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_END()                   \
              }                                                   \
            }                                                     \
          }                                                       \
        }                                                           \
      } while (0)


/* Use these to access the current edge inside the above macros */
#define STINGER_EDGE_SOURCE source__
#define STINGER_EDGE_TYPE type__
#define STINGER_EDGE_DEST ((current_edge__->neighbor)&(~STINGER_EDGE_DIRECTION_MASK))
#define STINGER_EDGE_DIRECTION ((current_edge__->neighbor)&(STINGER_EDGE_DIRECTION_MASK))
#define STINGER_EDGE_WEIGHT current_edge__->weight
#define STINGER_EDGE_TIME_FIRST current_edge__->timeFirst
#define STINGER_EDGE_TIME_RECENT current_edge__->timeRecent
#define STINGER_IS_OUT_EDGE ((current_edge__->neighbor)&(STINGER_EDGE_DIRECTION_OUT))
#define STINGER_IS_IN_EDGE ((current_edge__->neighbor)&(STINGER_EDGE_DIRECTION_IN))

#define STINGER_RO_EDGE_SOURCE source__
#define STINGER_RO_EDGE_TYPE ebp__[ebp_k__].etype
#define STINGER_RO_EDGE_DEST ((local_current_edge__.neighbor) & (~STINGER_EDGE_DIRECTION_MASK))
#define STINGER_RO_EDGE_DIRECTION ((local_current_edge__.neighbor)&(STINGER_EDGE_DIRECTION_MASK))
#define STINGER_RO_EDGE_WEIGHT local_current_edge__.weight
#define STINGER_RO_EDGE_TIME_FIRST local_current_edge__.timeFirst
#define STINGER_RO_EDGE_TIME_RECENT local_current_edge__.timeRecent
#define STINGER_RO_IS_OUT_EDGE ((local_current_edge__.neighbor)&(STINGER_EDGE_DIRECTION_OUT))
#define STINGER_RO_IS_IN_EDGE ((local_current_edge__.neighbor)&(STINGER_EDGE_DIRECTION_IN))


#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* STINGER_TRAVERSAL_H_ */

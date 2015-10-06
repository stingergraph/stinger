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

/* edges are writeable */
/* source vertex based */
#define STINGER_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_)		\
  do {									\
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    struct stinger_eb *  current_eb__ = ebpool_priv + stinger_vertex_edges_get(vertices, VTX_);	\
    while(current_eb__ != ebpool_priv) {						\
      int64_t source__ = current_eb__->vertexID;			\
      int64_t type__ = current_eb__->etype;				\
      for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
        if(!stinger_eb_is_blank(current_eb__, i__)) {			\
          struct stinger_edge * current_edge__ = current_eb__->edges + i__;

#define STINGER_FORALL_EDGES_OF_VTX_END()				\
        }								\
      }									\
      current_eb__ = ebpool_priv + (current_eb__->next);				\
    }									\
  } while (0)

#define STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(STINGER_,VTX_)    \
  do {                  \
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    struct stinger_eb *  current_eb__ = ebpool_priv + stinger_vertex_edges_get(vertices, VTX_); \
    while(current_eb__ != ebpool_priv) {            \
      int64_t source__ = current_eb__->vertexID;      \
      int64_t type__ = current_eb__->etype;       \
      for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
        if(!stinger_eb_is_blank(current_eb__, i__)) {     \
          struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
          if (STINGER_IS_OUT_EDGE) {

#define STINGER_FORALL_OUT_EDGES_OF_VTX_END()       \
          }   \
        }               \
      }                 \
      current_eb__ = ebpool_priv + (current_eb__->next);        \
    }                 \
  } while (0)

#define STINGER_FORALL_IN_EDGES_OF_VTX_BEGIN(STINGER_,VTX_)    \
  do {                  \
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    struct stinger_eb *  current_eb__ = ebpool_priv + stinger_vertex_edges_get(vertices, VTX_); \
    while(current_eb__ != ebpool_priv) {            \
      int64_t source__ = current_eb__->vertexID;      \
      int64_t type__ = current_eb__->etype;       \
      for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
        if(!stinger_eb_is_blank(current_eb__, i__)) {     \
          struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
          if (STINGER_IS_IN_EDGE) {

#define STINGER_FORALL_IN_EDGES_OF_VTX_END()       \
          } \
        }               \
      }                 \
      current_eb__ = ebpool_priv + (current_eb__->next);        \
    }                 \
  } while (0)

#define STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  do {									\
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    struct stinger_eb *  current_eb__ = ebpool_priv + stinger_vertex_edges_get(vertices, VTX_);	\
    while(current_eb__ != ebpool_priv) {						\
      int64_t source__ = current_eb__->vertexID;			\
      int64_t type__ = current_eb__->etype;				\
      if(current_eb__->etype == TYPE_) {				\
    	  for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
    	    if(!stinger_eb_is_blank(current_eb__, i__)) {               \
    	      struct stinger_edge * current_edge__ = current_eb__->edges + i__;

#define STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_END() \
          }								\
        }								\
      }									\
      current_eb__ = ebpool_priv+ (current_eb__->next);				\
    }									\
  } while (0)

#define STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  do {                  \
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    struct stinger_eb *  current_eb__ = ebpool_priv + stinger_vertex_edges_get(vertices, VTX_); \
    while(current_eb__ != ebpool_priv) {            \
      int64_t source__ = current_eb__->vertexID;      \
      int64_t type__ = current_eb__->etype;       \
      if(current_eb__->etype == TYPE_) {        \
        for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
          if(!stinger_eb_is_blank(current_eb__, i__)) {               \
            struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
            if (STINGER_IS_OUT_EDGE) {

#define STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END() \
            }           \
          }             \
        }               \
      }                 \
      current_eb__ = ebpool_priv+ (current_eb__->next);       \
    }                 \
  } while (0)

#define STINGER_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  do {                  \
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    struct stinger_eb *  current_eb__ = ebpool_priv + stinger_vertex_edges_get(vertices, VTX_); \
    while(current_eb__ != ebpool_priv) {            \
      int64_t source__ = current_eb__->vertexID;      \
      int64_t type__ = current_eb__->etype;       \
      if(current_eb__->etype == TYPE_) {        \
        for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
          if(!stinger_eb_is_blank(current_eb__, i__)) {               \
            struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
            if (STINGER_IS_IN_EDGE) {

#define STINGER_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END() \
            } \
          }               \
        }               \
      }                 \
      current_eb__ = ebpool_priv+ (current_eb__->next);       \
    }                 \
  } while (0)

#define STINGER_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_)	\
  do {									\
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    struct stinger_eb *  current_eb__ = ebpool_priv+ stinger_vertex_edges_get(vertices, VTX_);	\
    while(current_eb__ != ebpool_priv) {						\
      int64_t source__ = current_eb__->vertexID;			\
      int64_t type__ = current_eb__->etype;				\
      OMP("omp parallel for")						\
      MTA("mta assert parallel")					\
      for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
        if(!stinger_eb_is_blank(current_eb__, i__)) {                   \
          struct stinger_edge * current_edge__ = current_eb__->edges + i__;

#define STINGER_PARALLEL_FORALL_EDGES_OF_VTX_END()			\
        }								\
      }									\
      current_eb__ = ebpool_priv+ (current_eb__->next);				\
    }									\
  } while (0)

#define STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  do {                  \
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    struct stinger_eb *  current_eb__ = ebpool_priv+ stinger_vertex_edges_get(vertices, VTX_);  \
    while(current_eb__ != ebpool_priv) {            \
      int64_t source__ = current_eb__->vertexID;      \
      int64_t type__ = current_eb__->etype;       \
      OMP("omp parallel for")           \
      MTA("mta assert parallel")          \
      for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
        if(!stinger_eb_is_blank(current_eb__, i__)) {                   \
          struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
          if (STINGER_IS_OUT_EDGE) {

#define STINGER_PARALLEL_FORALL_OUT_EDGES_OF_VTX_END()      \
          } \
        }               \
      }                 \
      current_eb__ = ebpool_priv+ (current_eb__->next);       \
    }                 \
  } while (0)

#define STINGER_PARALLEL_FORALL_IN_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  do {                  \
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    struct stinger_eb *  current_eb__ = ebpool_priv+ stinger_vertex_edges_get(vertices, VTX_);  \
    while(current_eb__ != ebpool_priv) {            \
      int64_t source__ = current_eb__->vertexID;      \
      int64_t type__ = current_eb__->etype;       \
      OMP("omp parallel for")           \
      MTA("mta assert parallel")          \
      for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
        if(!stinger_eb_is_blank(current_eb__, i__)) {                   \
          struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
          if (STINGER_IS_IN_EDGE) {

#define STINGER_PARALLEL_FORALL_IN_EDGES_OF_VTX_END()      \
          } \
        }               \
      }                 \
      current_eb__ = ebpool_priv+ (current_eb__->next);       \
    }                 \
  } while (0)

#define STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  do {									\
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    struct stinger_eb *  current_eb__ = ebpool_priv+ stinger_vertex_edges_get(vertices, VTX_);	\
    while(current_eb__ != ebpool_priv) {						\
      int64_t source__ = current_eb__->vertexID;			\
      int64_t type__ = current_eb__->etype;				\
      if(current_eb__->etype == TYPE_) {				\
        OMP("omp parallel for")						\
    	  MTA("mta assert parallel")					\
    	  for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
    	    if(!stinger_eb_is_blank(current_eb__, i__)) {               \
    	      struct stinger_edge * current_edge__ = current_eb__->edges + i__;

#define STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END() \
          }								\
        }								\
      }									\
      current_eb__ = ebpool_priv+ (current_eb__->next);				\
    }									\
  } while (0)

#define STINGER_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  do {                  \
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    struct stinger_eb *  current_eb__ = ebpool_priv+ stinger_vertex_edges_get(vertices, VTX_);  \
    while(current_eb__ != ebpool_priv) {            \
      int64_t source__ = current_eb__->vertexID;      \
      int64_t type__ = current_eb__->etype;       \
      if(current_eb__->etype == TYPE_) {        \
        OMP("omp parallel for")           \
        MTA("mta assert parallel")          \
        for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
          if(!stinger_eb_is_blank(current_eb__, i__)) {               \
            struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
            if (STINGER_IS_OUT_EDGE) {

#define STINGER_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END() \
            } \
          }               \
        }               \
      }                 \
      current_eb__ = ebpool_priv+ (current_eb__->next);       \
    }                 \
  } while (0)

#define STINGER_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  do {                  \
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    struct stinger_eb *  current_eb__ = ebpool_priv+ stinger_vertex_edges_get(vertices, VTX_);  \
    while(current_eb__ != ebpool_priv) {            \
      int64_t source__ = current_eb__->vertexID;      \
      int64_t type__ = current_eb__->etype;       \
      if(current_eb__->etype == TYPE_) {        \
        OMP("omp parallel for")           \
        MTA("mta assert parallel")          \
        for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
          if(!stinger_eb_is_blank(current_eb__, i__)) {               \
            struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
            if (STINGER_IS_IN_EDGE) {

#define STINGER_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END() \
            } \
          }               \
        }               \
      }                 \
      current_eb__ = ebpool_priv+ (current_eb__->next);       \
    }                 \
  } while (0)


/* all edges of a given type */
#define STINGER_FORALL_EDGES_BEGIN(STINGER_,TYPE_) \
  do {									\
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    for(uint64_t p__ = 0; p__ < ETA((STINGER_),(TYPE_))->high; p__++) {	\
      struct stinger_eb *  current_eb__ = ebpool_priv+ ETA((STINGER_),(TYPE_))->blocks[p__]; \
      int64_t source__ = current_eb__->vertexID;			\
      int64_t type__ = current_eb__->etype;				\
      for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
        if(!stinger_eb_is_blank(current_eb__, i__)) {                   \
          struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
          if (STINGER_IS_OUT_EDGE) {

#define STINGER_FORALL_EDGES_END()					\
          }  \
        }								\
      }									\
    }									\
  } while (0)

  /* all edges */
#define STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(STINGER_) \
  do {                  \
    MAP_STING(STINGER_); \
    for (uint64_t t__ = 0; t__ < stinger_max_num_etypes(STINGER_); t__++) { \
      struct stinger_eb * ebpool_priv = ebpool->ebpool; \
      for(uint64_t p__ = 0; p__ < ETA((STINGER_),(t__))->high; p__++) { \
        struct stinger_eb *  current_eb__ = ebpool_priv+ ETA((STINGER_),(t__))->blocks[p__]; \
        int64_t source__ = current_eb__->vertexID;      \
        int64_t type__ = current_eb__->etype;       \
        for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
          if(!stinger_eb_is_blank(current_eb__, i__)) {                   \
            struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
            if (STINGER_IS_OUT_EDGE) {

#define STINGER_FORALL_EDGES_OF_ALL_TYPES_END()          \
            } \
          }               \
        }                 \
      }                 \
    }                   \
  } while (0)

#define STINGER_PARALLEL_FORALL_EDGES_BEGIN(STINGER_,TYPE_)		\
  do {									\
    MAP_STING(STINGER_); \
    struct stinger_eb * ebpool_priv = ebpool->ebpool; \
    OMP("omp parallel for")						\
    MTA("mta assert parallel")						\
    for(uint64_t p__ = 0; p__ < ETA((STINGER_),(TYPE_))->high; p__++) {	\
      struct stinger_eb *  current_eb__ = ebpool_priv+ ETA((STINGER_),(TYPE_))->blocks[p__]; \
      int64_t source__ = current_eb__->vertexID;			\
      int64_t type__ = current_eb__->etype;				\
      for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
        if(!stinger_eb_is_blank(current_eb__, i__)) {                   \
          struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
          if (STINGER_IS_OUT_EDGE) {

#define STINGER_PARALLEL_FORALL_EDGES_END()				\
          } \
        }								\
      } 								\
    }									\
   } while (0)

  /* all edges */
#define STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_BEGIN(STINGER_) \
  do {                  \
    MAP_STING(STINGER_); \
    for (uint64_t t__ = 0; t__ < stinger_max_num_etypes(STINGER_); t__++) { \
      struct stinger_eb * ebpool_priv = ebpool->ebpool; \
      OMP("omp parallel for")           \
      MTA("mta assert parallel")            \
      for(uint64_t p__ = 0; p__ < ETA((STINGER_),(t__))->high; p__++) { \
        struct stinger_eb *  current_eb__ = ebpool_priv+ ETA((STINGER_),(t__))->blocks[p__]; \
        int64_t source__ = current_eb__->vertexID;      \
        int64_t type__ = current_eb__->etype;       \
        for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
          if(!stinger_eb_is_blank(current_eb__, i__)) {                   \
            struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
            if (STINGER_IS_OUT_EDGE) {

#define STINGER_PARALLEL_FORALL_EDGES_OF_ALL_TYPES_END()          \
            } \
          }               \
        }                 \
      }                 \
    }                   \
  } while (0)


/* read only */
/* source vertex based */
#define STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_)      \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);			\
    const struct stinger_eb * restrict ebp__ = ebpool->ebpool;	\
    const int64_t source__ = (VTX_);                                    \
    int64_t ebp_k__ = vertices->vertices[source__].edges;		\
    while(ebp_k__) {                                                    \
      MTA("mta assert parallel")                                        \
        for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {       \
          if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {              \
            const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
            if(local_current_edge__.neighbor >= 0) {

#define STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END()                     \
            }                                                           \
          }                                                             \
        }                                                               \
      ebp_k__ = ebp__[ebp_k__].next;                                    \
    }                                                                   \
  } while (0)

#define STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_VTX_BEGIN(STINGER_,VTX_)      \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);     \
    const struct stinger_eb * restrict ebp__ = ebpool->ebpool;  \
    const int64_t source__ = (VTX_);                                    \
    int64_t ebp_k__ = vertices->vertices[source__].edges;   \
    while(ebp_k__) {                                                    \
      MTA("mta assert parallel")                                        \
        for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {       \
          if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {              \
            const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
            if(local_current_edge__.neighbor >= 0) { \
              if (STINGER_RO_IS_OUT_EDGE) {


#define STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_VTX_END()                     \
              } \
            }                                                           \
          }                                                             \
        }                                                               \
      ebp_k__ = ebp__[ebp_k__].next;                                    \
    }                                                                   \
  } while (0)

#define STINGER_READ_ONLY_FORALL_IN_EDGES_OF_VTX_BEGIN(STINGER_,VTX_)      \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);     \
    const struct stinger_eb * restrict ebp__ = ebpool->ebpool;  \
    const int64_t source__ = (VTX_);                                    \
    int64_t ebp_k__ = vertices->vertices[source__].edges;   \
    while(ebp_k__) {                                                    \
      MTA("mta assert parallel")                                        \
        for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {       \
          if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {              \
            const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
            if(local_current_edge__.neighbor >= 0) { \
              if (STINGER_RO_IS_IN_EDGE) {

#define STINGER_READ_ONLY_FORALL_IN_EDGES_OF_VTX_END()                     \
              } \
            }                                                           \
          }                                                             \
        }                                                               \
      ebp_k__ = ebp__[ebp_k__].next;                                    \
    }                                                                   \
  } while (0)

#define STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_)      \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);			\
    const struct stinger_eb * restrict ebp__ = ebpool->ebpool;	\
    const int64_t source__ = (VTX_);                                    \
    const int64_t etype__ = (TYPE_);                                    \
    int64_t ebp_k__ = vertices->vertices[source__].edges;		\
    while(ebp_k__ && ebp__[ebp_k__].etype != etype__)                   \
      ebp_k__ = ebp__[ebp_k__].next;                                    \
    while(ebp_k__) {                 \
      if (ebp__[ebp_k__].etype == etype__) {                            \
        MTA("mta assert parallel")                                      \
          for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {     \
            if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {            \
              const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
              if(local_current_edge__.neighbor >= 0) {

#define STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_END()             \
              }                                                         \
            }                                                           \
          }                                                             \
      }                                                                 \
      ebp_k__ = ebp__[ebp_k__].next;                                    \
    }                                                                   \
  } while (0)

#define STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_)      \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);     \
    const struct stinger_eb * restrict ebp__ = ebpool->ebpool;  \
    const int64_t source__ = (VTX_);                                    \
    const int64_t etype__ = (TYPE_);                                    \
    int64_t ebp_k__ = vertices->vertices[source__].edges;   \
    while(ebp_k__ && ebp__[ebp_k__].etype != etype__)                   \
      ebp_k__ = ebp__[ebp_k__].next;                                    \
    while(ebp_k__) {                 \
      if (ebp__[ebp_k__].etype == etype__) {                            \
        MTA("mta assert parallel")                                      \
          for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {     \
            if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {            \
              const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
              if(local_current_edge__.neighbor >= 0) { \
                if (STINGER_RO_IS_OUT_EDGE) {

#define STINGER_READ_ONLY_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END()             \
                } \
              }                                                         \
            }                                                           \
          }                                                             \
      }                                                                 \
      ebp_k__ = ebp__[ebp_k__].next;                                    \
    }                                                                   \
  } while (0)

#define STINGER_READ_ONLY_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_)      \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);     \
    const struct stinger_eb * restrict ebp__ = ebpool->ebpool;  \
    const int64_t source__ = (VTX_);                                    \
    const int64_t etype__ = (TYPE_);                                    \
    int64_t ebp_k__ = vertices->vertices[source__].edges;   \
    while(ebp_k__ && ebp__[ebp_k__].etype != etype__)                   \
      ebp_k__ = ebp__[ebp_k__].next;                                    \
    while(ebp_k__) {                 \
      if (ebp__[ebp_k__].etype == etype__) {                            \
        MTA("mta assert parallel")                                      \
          for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {     \
            if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {            \
              const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
              if(local_current_edge__.neighbor >= 0) { \
                if (STINGER_RO_IS_IN_EDGE) {

#define STINGER_READ_ONLY_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END()             \
                } \
              }                                                         \
            }                                                           \
          }                                                             \
      }                                                                 \
      ebp_k__ = ebp__[ebp_k__].next;                                    \
    }                                                                   \
  } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);			\
    const struct stinger_eb * restrict ebp__ = ebpool->ebpool;	\
    const int64_t source__ = (VTX_);					\
    OMP("omp parallel") {                                               \
      OMP("omp single") {                                               \
        int64_t ebp_k__ = vertices->vertices[source__].edges;			\
        while(ebp_k__) {                                                \
          OMP("omp task untied firstprivate(ebp_k__)")                  \
            MTA("mta assert parallel")                                  \
            for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {   \
              if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {          \
                const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
                if(local_current_edge__.neighbor >= 0) {

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END()  \
                }                                             \
              }                                               \
            }                                                 \
          ebp_k__ = ebp__[ebp_k__].next;                      \
        }                                                     \
      }                                                       \
    } OMP("omp taskwait");                                    \
  } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);     \
    const struct stinger_eb * restrict ebp__ = ebpool->ebpool;  \
    const int64_t source__ = (VTX_);          \
    OMP("omp parallel") {                                               \
      OMP("omp single") {                                               \
        int64_t ebp_k__ = vertices->vertices[source__].edges;     \
        while(ebp_k__) {                                                \
          OMP("omp task untied firstprivate(ebp_k__)")                  \
            MTA("mta assert parallel")                                  \
            for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {   \
              if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {          \
                const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
                if(local_current_edge__.neighbor >= 0) { \
                  if (STINGER_RO_IS_OUT_EDGE) {

#define STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_VTX_END()  \
                  } \
                }                                             \
              }                                               \
            }                                                 \
          ebp_k__ = ebp__[ebp_k__].next;                      \
        }                                                     \
      }                                                       \
    } OMP("omp taskwait");                                    \
  } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_VTX_BEGIN(STINGER_,VTX_) \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);     \
    const struct stinger_eb * restrict ebp__ = ebpool->ebpool;  \
    const int64_t source__ = (VTX_);          \
    OMP("omp parallel") {                                               \
      OMP("omp single") {                                               \
        int64_t ebp_k__ = vertices->vertices[source__].edges;     \
        while(ebp_k__) {                                                \
          OMP("omp task untied firstprivate(ebp_k__)")                  \
            MTA("mta assert parallel")                                  \
            for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {   \
              if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {          \
                const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
                if(local_current_edge__.neighbor >= 0) { \
                  if (STINGER_RO_IS_IN_EDGE) {

#define STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_VTX_END()  \
                  } \
                }                                             \
              }                                               \
            }                                                 \
          ebp_k__ = ebp__[ebp_k__].next;                      \
        }                                                     \
      }                                                       \
    } OMP("omp taskwait");                                    \
  } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);			\
    const struct stinger_eb * ebpool_priv = ebpool->ebpool;		\
    OMP("omp parallel") {                                               \
      const struct stinger_eb * restrict ebp__ = ebpool->ebpool;                      \
      const int64_t source__ = (VTX_);                                  \
      const int64_t etype__ = (TYPE_);                                  \
      OMP("omp single") {                                               \
        int64_t ebp_k__ = vertices->vertices[source__].edges;			\
        while(ebp_k__ && ebp__[ebp_k__].etype != etype__)               \
          ebp_k__ = ebp__[ebp_k__].next;                                \
        while(ebp_k__ && ebp__[ebp_k__].etype == etype__) {             \
          OMP("omp task untied firstprivate(ebp_k__)")                  \
            MTA("mta assert parallel")                                  \
            for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {   \
              if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {          \
                const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
                if(local_current_edge__.neighbor >= 0) {

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END()  \
                }                                             \
              }                                               \
            }                                                 \
          ebp_k__ = ebp__[ebp_k__].next;                      \
        }                                                     \
      }                                                       \
    } OMP("omp taskwait");                                    \
  } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);     \
    const struct stinger_eb * ebpool_priv = ebpool->ebpool;   \
    OMP("omp parallel") {                                               \
      const struct stinger_eb * restrict ebp__ = ebpool->ebpool;                      \
      const int64_t source__ = (VTX_);                                  \
      const int64_t etype__ = (TYPE_);                                  \
      OMP("omp single") {                                               \
        int64_t ebp_k__ = vertices->vertices[source__].edges;     \
        while(ebp_k__ && ebp__[ebp_k__].etype != etype__)               \
          ebp_k__ = ebp__[ebp_k__].next;                                \
        while(ebp_k__ && ebp__[ebp_k__].etype == etype__) {             \
          OMP("omp task untied firstprivate(ebp_k__)")                  \
            MTA("mta assert parallel")                                  \
            for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {   \
              if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {          \
                const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
                if(local_current_edge__.neighbor >= 0) { \
                  if (STINGER_RO_IS_OUT_EDGE) {

#define STINGER_READ_ONLY_PARALLEL_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END()  \
                  } \
                }                                             \
              }                                               \
            }                                                 \
          ebp_k__ = ebp__[ebp_k__].next;                      \
        }                                                     \
      }                                                       \
    } OMP("omp taskwait");                                    \
  } while (0)

#define STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,TYPE_,VTX_) \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);     \
    const struct stinger_eb * ebpool_priv = ebpool->ebpool;   \
    OMP("omp parallel") {                                               \
      const struct stinger_eb * restrict ebp__ = ebpool->ebpool;                      \
      const int64_t source__ = (VTX_);                                  \
      const int64_t etype__ = (TYPE_);                                  \
      OMP("omp single") {                                               \
        int64_t ebp_k__ = vertices->vertices[source__].edges;     \
        while(ebp_k__ && ebp__[ebp_k__].etype != etype__)               \
          ebp_k__ = ebp__[ebp_k__].next;                                \
        while(ebp_k__ && ebp__[ebp_k__].etype == etype__) {             \
          OMP("omp task untied firstprivate(ebp_k__)")                  \
            MTA("mta assert parallel")                                  \
            for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) {   \
              if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {          \
                const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
                if(local_current_edge__.neighbor >= 0) { \
                  if (STINGER_RO_IS_IN_EDGE) {

#define STINGER_READ_ONLY_PARALLEL_FORALL_IN_EDGES_OF_TYPE_OF_VTX_END()  \
                  } \
                }                                             \
              }                                               \
            }                                                 \
          ebp_k__ = ebp__[ebp_k__].next;                      \
        }                                                     \
      }                                                       \
    } OMP("omp taskwait");                                    \
  } while (0)


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

#if !defined(STINGER_TRAVERSAL_H_)
#define STINGER_TRAVERSAL_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include "stinger_internal.h"

#undef STINGER_FORALL_EDGES_OF_VTX_BEGIN
#undef STINGER_FORALL_EDGES_OF_VTX_END

#undef STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN
#undef STINGER_PARALLEL_FORALL_EDGES_OF_VTX_END

#undef STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_FORALL_EDGES_BEGIN
#undef STINGER_FORALL_EDGES_END

#undef STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN
#undef STINGER_FORALL_EDGES_OF_ALL_TYPES_END

#undef STINGER_PARALLEL_FORALL_EDGES_BEGIN
#undef STINGER_PARALLEL_FORALL_EDGES_END

#undef STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_FORALL_EDGES_OF_VTX_END

#undef STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_FORALL_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_VTX_END

#undef STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN
#undef STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_END

#undef STINGER_READ_ONLY_FORALL_EDGES_BEGIN
#undef STINGER_READ_ONLY_FORALL_EDGES_END

#undef STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_BEGIN
#undef STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_END

#undef OF_VERTICES
#undef OF_EDGE_TYPES
#undef OF_VERTEX_TYPES
#undef MODIFIED_BEFORE
#undef MODIFIED_AFTER
#undef CREATED_BEFORE
#undef CREATED_AFTER
#undef STINGER_TRAVERSE_EDGES

/* Use these to access the current edge inside the above macros */
#undef STINGER_EDGE_SOURCE
#undef STINGER_EDGE_TYPE
#undef STINGER_EDGE_DEST
#undef STINGER_EDGE_WEIGHT
#undef STINGER_EDGE_TIME_FIRST
#undef STINGER_EDGE_TIME_RECENT

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
	  struct stinger_edge * current_edge__ = current_eb__->edges + i__;

#define STINGER_FORALL_EDGES_END()					\
	}								\
      }									\
    }									\
  } while (0)

  /* all edges */
#define STINGER_FORALL_EDGES_OF_ALL_TYPES_BEGIN(STINGER_) \
  do {                  \
    MAP_STING(STINGER_); \
    for (uint64_t t__ = 1; t__ < stinger_max_num_etypes(STINGER_); t__++) { \
      struct stinger_eb * ebpool_priv = ebpool->ebpool; \
      for(uint64_t p__ = 0; p__ < ETA((STINGER_),(t__))->high; p__++) { \
        struct stinger_eb *  current_eb__ = ebpool_priv+ ETA((STINGER_),(t__))->blocks[p__]; \
        int64_t source__ = current_eb__->vertexID;      \
        int64_t type__ = current_eb__->etype;       \
        for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
          if(!stinger_eb_is_blank(current_eb__, i__)) {                   \
            struct stinger_edge * current_edge__ = current_eb__->edges + i__;

#define STINGER_FORALL_EDGES_OF_ALL_TYPES_END()          \
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
	  struct stinger_edge * current_edge__ = current_eb__->edges + i__;

#define STINGER_PARALLEL_FORALL_EDGES_END()				\
	}								\
      } 								\
    }									\
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
    while(ebp_k__ && ebp__[ebp_k__].etype == etype__) {                 \
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
        ebp_k__ = ebp__[ebp_k__].next;                                  \
      }                                                                 \
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

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(STINGER_,VTX_) \
  do {                                                                  \
    CONST_MAP_STING(STINGER_); \
    const struct stinger * restrict S__ = (STINGER_);			\
    const struct stinger_eb * ebpool_priv = ebpool->ebpool;		\
    OMP("omp parallel") {                                               \
      struct stinger_eb * restrict ebp__ = ebpool;                      \
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
        OMP("omp parallel") {                                           \
          OMP("omp single") {                                           \
            for(uint64_t p__ = 0; p__ < ETA((STINGER_),(TYPE_))->high; p__++) { \
              int64_t ebp_k__ = ETA((STINGER_),(TYPE_))->blocks[p__];          \
              const int64_t source__ = ebp__[ebp_k__].vertexID;         \
              const int64_t type__ = ebp__[ebp_k__].etype;              \
              OMP("omp task untied firstprivate(ebp_k__)")              \
                for(uint64_t i__ = 0; i__ < ebp__[ebp_k__].high; i__++) { \
                  if(!stinger_eb_is_blank(&ebp__[ebp_k__], i__)) {      \
		    const struct stinger_edge local_current_edge__ = ebp__[ebp_k__].edges[i__]; \
                    if(local_current_edge__.neighbor >= 0) {

#define STINGER_READ_ONLY_PARALLEL_FORALL_EDGES_END()                   \
                    }                                                   \
                  }                                                     \
                }                                                       \
            }                                                           \
            OMP("omp taskwait");                                        \
          }                                                             \
        }                                                               \
      } while (0)


/* Use these to access the current edge inside the above macros */
#define STINGER_EDGE_SOURCE source__
#define STINGER_EDGE_TYPE type__
#define STINGER_EDGE_DEST current_edge__->neighbor
#define STINGER_EDGE_WEIGHT current_edge__->weight
#define STINGER_EDGE_TIME_FIRST current_edge__->timeFirst
#define STINGER_EDGE_TIME_RECENT current_edge__->timeRecent

#define STINGER_RO_EDGE_SOURCE source__
#define STINGER_RO_EDGE_TYPE ebp__[ebp_k__].type
#define STINGER_RO_EDGE_DEST local_current_edge__.neighbor
#define STINGER_RO_EDGE_WEIGHT local_current_edge__.weight
#define STINGER_RO_EDGE_TIME_FIRST local_current_edge__.timeFirst
#define STINGER_RO_EDGE_TIME_RECENT local_current_edge__.timeRecent

#define OF_VERTICES(array, count) vtx_filter = (array); vtx_filter_count = (count);  filt_mask |= 0x1;
#define OF_EDGE_TYPES(array, count) edge_type_filter = (array); edge_type_filter_count = (count); filt_mask |= 0x2;
#define OF_VERTEX_TYPES(array, count) vtx_type_filter = (array); vtx_type_filter_count = (count); filt_mask |= 0x4;
#define MODIFIED_BEFORE(time) modified_before = time; filt_mask |= 0x8;
#define MODIFIED_AFTER(time) modified_after = time; filt_mask |= 0x8;
#define CREATED_BEFORE(time) created_before = time; filt_mask |= 0x8;
#define CREATED_AFTER(time) created_after = time; filt_mask |= 0x8;
#define STINGER_TRAVERSE_EDGES(stinger, filter, code) { \
  int filt_mask = 0; \
  int64_t modified_before = INT64_MAX, modified_after = INT64_MIN; \
  int64_t created_before = INT64_MAX, created_after = INT64_MIN; \
  uint64_t * vtx_filter, * edge_type_filter, * vtx_type_filter; \
  uint64_t vtx_filter_count, edge_type_filter_count, vtx_type_filter_count; \
  { filter }\
  MAP_STING(STINGER_); \
  struct stinger_eb * ebpool_priv = ebpool->ebpool; \
  switch(filt_mask) { \
    default: { \
      printf("WARNING: No filter provided\n"); \
    } break; \
    /* vtx */  \
    case 1: { \
      for(uint64_t j__ = 0; j__ < vtx_filter_count; j__++) { \
	STINGER_FORALL_EDGES_OF_VTX_BEGIN((stinger), vtx_filter[j__]) { \
	  { code } \
	} STINGER_FORALL_EDGES_OF_VTX_END(); \
      } \
    } break; \
    /* etype */ \
    case 2: { \
      for(uint64_t j__ = 0; j__ < edge_type_filter_count; j__++) { \
	STINGER_FORALL_EDGES_BEGIN((stinger), edge_type_filter[j__]) { \
	  { code } \
	} STINGER_FORALL_EDGES_END(); \
      } \
    } break; \
    /* vtx + etype */ \
    case 3: { \
      for(uint64_t k__ = 0; k__ < vtx_filter_count; k__++) { \
	struct stinger_eb *  current_eb__ = vertices->vertices[(vtx_filter[k__])].edges;	\
	while(current_eb__ != ebpool_priv) {						\
	  int64_t source__ = current_eb__->vertexID;			\
	  int64_t type__ = current_eb__->etype;				\
	  for(uint64_t j__ = 0; j__ < edge_type_filter_count; j__++) { \
	    if(current_eb__->etype == edge_type_filter[j__]) {				\
	      for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
		if(!stinger_eb_is_blank(current_eb__, i__)) {               \
		  struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
		  { code } \
		}								\
	      }								\
	      break; \
	    }									\
	  } \
	  current_eb__ = ebpool_priv+ (current_eb__->next);				\
	}									\
      } \
    } break; \
    /* vtype */ \
    case 4: { \
      for(uint64_t j__ = 0; j__ < (stinger->max_nv); j__++) { \
	for(uint64_t i__ = 0; i__ < vtx_type_filter_count; i__++) { \
	  if(stinger_vtype((stinger), j__) == vtx_type_filter[i__]) { \
	    STINGER_FORALL_EDGES_OF_VTX_BEGIN(stinger, j__) { \
	      for(uint64_t k__ = 0; k__ < vtx_type_filter_count; k__++) { \
		if(stinger_vtype((stinger), STINGER_EDGE_DEST) == vtx_type_filter[k__]) { \
		  { code }  \
		  break; \
		} \
	      } \
	    } STINGER_FORALL_EDGES_OF_VTX_END(); \
	    break; \
	  } \
	} \
      } \
    } break; \
    /* vtx + vtype */ \
    case 5: { \
      for(uint64_t j__ = 0; j__ < vtx_filter_count; j__++) { \
	STINGER_FORALL_EDGES_OF_VTX_BEGIN((stinger), vtx_filter[j__]) { \
	  for(uint64_t k__ = 0; k__ < vtx_type_filter_count; k__++) { \
	    if(stinger_vtype((stinger), STINGER_EDGE_DEST) == vtx_type_filter[k__]) { \
	      { code }  \
	      break; \
	    } \
	  } \
	} STINGER_FORALL_EDGES_OF_VTX_END(); \
      } \
    } break; \
    /* etype + vtype */ \
    case 6: { \
      for(uint64_t j__ = 0; j__ < edge_type_filter_count; j__++) { \
	STINGER_FORALL_EDGES_BEGIN((stinger), edge_type_filter[j__]) { \
	  for(uint64_t k__ = 0; k__ < vtx_type_filter_count; k__++) { \
	    if(stinger_vtype((stinger), STINGER_EDGE_DEST) == vtx_type_filter[k__]) { \
	      { code }  \
	      break; \
	    } \
	  } \
	} STINGER_FORALL_EDGES_END(); \
      } \
    } break; \
    /* vtx + etype + vtype */ \
    case 7: { \
      for(uint64_t k__ = 0; k__ < vtx_filter_count; k__++) { \
	struct stinger_eb *  current_eb__ = vertices->vertices[(vtx_filter[k__])].edges;	\
	while(current_eb__ != ebpool_priv) {						\
	  int64_t source__ = current_eb__->vertexID;			\
	  int64_t type__ = current_eb__->etype;				\
	  for(uint64_t j__ = 0; j__ < edge_type_filter_count; j__++) { \
	    if(current_eb__->etype == edge_type_filter[j__]) {				\
	      for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
		if(!stinger_eb_is_blank(current_eb__, i__)) {               \
		  struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
		  for(uint64_t p__ = 0; p__ < vtx_type_filter_count; p__++) { \
		    if(stinger_vtype((stinger), STINGER_EDGE_DEST) == vtx_type_filter[p__]) { \
		      { code }  \
		      break; \
		    } \
		  } \
		}								\
	      }								\
	      break; \
	    }									\
	  } \
	  current_eb__ = ebpool_priv+ (current_eb__->next);				\
	}									\
      } \
    } break; \
    case 8: { \
      for(uint64_t j__ = 0; j__ < (stinger->max_nv); j__++) { \
	STINGER_FORALL_EDGES_OF_VTX_BEGIN(stinger, j__) { \
	  if(STINGER_EDGE_TIME_FIRST > created_after && STINGER_EDGE_TIME_FIRST < created_before && \
	     STINGER_EDGE_TIME_RECENT > modified_after && STINGER_EDGE_TIME_RECENT < modified_before) { \
	    { code }  \
	  }                                                                 \
	} STINGER_FORALL_EDGES_OF_VTX_END(); \
      } \
    } break; \
    case 9: { \
      for(uint64_t j__ = 0; j__ < vtx_filter_count; j__++) { \
	STINGER_FORALL_EDGES_OF_VTX_BEGIN((stinger), vtx_filter[j__]) { \
	  if(STINGER_EDGE_TIME_FIRST > created_after && STINGER_EDGE_TIME_FIRST < created_before && \
	     STINGER_EDGE_TIME_RECENT > modified_after && STINGER_EDGE_TIME_RECENT < modified_before) { \
	    { code } \
	  } \
	} STINGER_FORALL_EDGES_OF_VTX_END(); \
      } \
    } break; \
    /* etype */ \
    case 10: { \
      for(uint64_t j__ = 0; j__ < edge_type_filter_count; j__++) { \
	STINGER_FORALL_EDGES_BEGIN((stinger), edge_type_filter[j__]) { \
	  if(STINGER_EDGE_TIME_FIRST > created_after && STINGER_EDGE_TIME_FIRST < created_before && \
	     STINGER_EDGE_TIME_RECENT > modified_after && STINGER_EDGE_TIME_RECENT < modified_before) { \
	    { code } \
	  } \
	} STINGER_FORALL_EDGES_END(); \
      } \
    } break; \
    /* vtx + etype */ \
    case 11: { \
      for(uint64_t k__ = 0; k__ < vtx_filter_count; k__++) { \
	struct stinger_eb *  current_eb__ = vertices->vertices[(vtx_filter[k__])].edges;	\
	while(current_eb__ != ebpool_priv) {						\
	  int64_t source__ = current_eb__->vertexID;			\
	  int64_t type__ = current_eb__->etype;				\
	  for(uint64_t j__ = 0; j__ < edge_type_filter_count; j__++) { \
	    if(current_eb__->etype == edge_type_filter[j__]) {				\
	      for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
		if(!stinger_eb_is_blank(current_eb__, i__)) {               \
		  struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
		  if(STINGER_EDGE_TIME_FIRST > created_after && STINGER_EDGE_TIME_FIRST < created_before && \
		     STINGER_EDGE_TIME_RECENT > modified_after && STINGER_EDGE_TIME_RECENT < modified_before) { \
		    { code } \
		  } \
		}								\
	      }								\
	      break; \
	    }									\
	  } \
	  current_eb__ = ebpool_priv+ (current_eb__->next);				\
	}									\
      } \
    } break; \
    /* vtype */ \
    case 12: { \
      for(uint64_t j__ = 0; j__ < (stinger->max_nv); j__++) { \
	for(uint64_t i__ = 0; i__ < vtx_type_filter_count; i__++) { \
	  if(stinger_vtype((stinger), j__) == vtx_type_filter[i__]) { \
	    STINGER_FORALL_EDGES_OF_VTX_BEGIN(stinger, j__) { \
	      for(uint64_t k__ = 0; k__ < vtx_type_filter_count; k__++) { \
		if(stinger_vtype((stinger), STINGER_EDGE_DEST) == vtx_type_filter[k__]) { \
		  if(STINGER_EDGE_TIME_FIRST > created_after && STINGER_EDGE_TIME_FIRST < created_before && \
		     STINGER_EDGE_TIME_RECENT > modified_after && STINGER_EDGE_TIME_RECENT < modified_before) { \
		    { code }  \
		  } \
		  break; \
		} \
	      } \
	    } STINGER_FORALL_EDGES_OF_VTX_END(); \
	    break; \
	  } \
	} \
      } \
    } break; \
    /* vtx + vtype */ \
    case 13: { \
      for(uint64_t j__ = 0; j__ < vtx_filter_count; j__++) { \
	STINGER_FORALL_EDGES_OF_VTX_BEGIN((stinger), vtx_filter[j__]) { \
	  for(uint64_t k__ = 0; k__ < vtx_type_filter_count; k__++) { \
	    if(stinger_vtype((stinger), STINGER_EDGE_DEST) == vtx_type_filter[k__]) { \
	      if(STINGER_EDGE_TIME_FIRST > created_after && STINGER_EDGE_TIME_FIRST < created_before && \
		 STINGER_EDGE_TIME_RECENT > modified_after && STINGER_EDGE_TIME_RECENT < modified_before) { \
		{ code }  \
	      } \
	      break; \
	    } \
	  } \
	} STINGER_FORALL_EDGES_OF_VTX_END(); \
      } \
    } break; \
    /* etype + vtype */ \
    case 14: { \
      for(uint64_t j__ = 0; j__ < edge_type_filter_count; j__++) { \
	STINGER_FORALL_EDGES_BEGIN((stinger), edge_type_filter[j__]) { \
	  for(uint64_t k__ = 0; k__ < vtx_type_filter_count; k__++) { \
	    if(stinger_vtype((stinger), STINGER_EDGE_DEST) == vtx_type_filter[k__]) { \
	      if(STINGER_EDGE_TIME_FIRST > created_after && STINGER_EDGE_TIME_FIRST < created_before && \
		 STINGER_EDGE_TIME_RECENT > modified_after && STINGER_EDGE_TIME_RECENT < modified_before) { \
		{ code }  \
	      } \
	      break; \
	    } \
	  } \
	} STINGER_FORALL_EDGES_END(); \
      } \
    } break; \
    /* vtx + etype + vtype */ \
    case 15: { \
      for(uint64_t k__ = 0; k__ < vtx_filter_count; k__++) { \
	struct stinger_eb *  current_eb__ = vertices->vertices[(vtx_filter[k__])].edges;	\
	while(current_eb__ != ebpool_priv) {						\
	  int64_t source__ = current_eb__->vertexID;			\
	  int64_t type__ = current_eb__->etype;				\
	  for(uint64_t j__ = 0; j__ < edge_type_filter_count; j__++) { \
	    if(current_eb__->etype == edge_type_filter[j__]) {				\
	      for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
		if(!stinger_eb_is_blank(current_eb__, i__)) {               \
		  struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
		  for(uint64_t p__ = 0; p__ < vtx_type_filter_count; p__++) { \
		    if(stinger_vtype((stinger), STINGER_EDGE_DEST) == vtx_type_filter[p__]) { \
		      if(STINGER_EDGE_TIME_FIRST > created_after && STINGER_EDGE_TIME_FIRST < created_before && \
			 STINGER_EDGE_TIME_RECENT > modified_after && STINGER_EDGE_TIME_RECENT < modified_before) { \
			{ code }  \
		      } \
		      break; \
		    } \
		  } \
		}								\
	      }								\
	      break; \
	    }									\
	  } \
	  current_eb__ = ebpool_priv+ (current_eb__->next);				\
	}									\
      } \
    } break; \
  } \
} 

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* STINGER_TRAVERSAL_H_ */

#ifndef STINGER_DEPRECATED_H
#define STINGER_DEPRECATED_H

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include <stdint.h>

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
#define OF_VERTICES(ARRAY_, COUNT_)
#define OF_VERTEX_TYPE(ARRAY_, COUNT_)
#define OF_EDGE_TYPE(ARRAY_, COUNT_)
#define MODIFIED_BEFORE(TIME_)
#define MODIFIED_AFTER(TIME_)
#define CREATED_BEFORE(TIME_)
#define CREATED_AFTER(TIME_)
#define STINGER_TRAVERSE_EDGES(STINGER_, FILTER_, CODE_)

#undef OF_VERTICES
#undef OF_EDGE_TYPES
#undef OF_VERTEX_TYPES
#undef MODIFIED_BEFORE
#undef MODIFIED_AFTER
#undef CREATED_BEFORE
#undef CREATED_AFTER
#undef STINGER_TRAVERSE_EDGES

/* These functions are included for backwards compatability, but are not recommended
 * for use.  No guarantees are made about their performance, and they are no longer
 * part of the STINGER API.
 */

void update_edge_data (struct stinger * S, struct stinger_eb *eb,
                uint64_t index, int64_t neighbor, int64_t weight, int64_t ts, int64_t operation);

int
stinger_remove_and_insert_edges (struct stinger *G,
                                 int64_t type, int64_t from,
                                 int64_t nremove, int64_t * remove,
                                 int64_t ninsert, int64_t * insert,
                                 int64_t * weight, int64_t timestamp);

int64_t
stinger_remove_and_insert_batch (struct stinger * G, int64_t type,
                                 int64_t timestamp, int64_t n,
                                 int64_t * insoff, int64_t * deloff,
                                 int64_t * act);
void
stinger_gather_typed_successors_serial (const struct stinger *G, int64_t type,
                                        int64_t v, size_t * outlen,
                                        int64_t * out, size_t max_outlen);

#define stinger_outdegree(S,Y)	stinger_outdegree_get(S,Y)
#define stinger_indegree(S,Y)	stinger_indegree_get(S,Y)
#define stinger_vtype(S,Y)	stinger_vtype_get(S,Y)

#if defined(STINGER_FORCE_OLD_MAP)
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * OLD STINGER_PHYSMAP.H
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MAX_VTXID 0xFFFFF
/** \def MAX_VTXID
*   \brief Maximum number of vertices the physical mapper can support.
*   Note: If the physical mapper produces errors, increase this number.
*/
#define MAX_NODES 0xFFFFF
/** \def MAX_NODES
*   \brief Maximum number internal nodes that the physical mapper can support.
*   Note: If the physical mapper produces errors, increase this number.
*/

typedef struct stinger_physmap stinger_physmap_t;

stinger_physmap_t * 
stinger_physmap_create();

void
stinger_physmap_delete(stinger_physmap_t * map);

uint64_t
stinger_physmap_create_mapping (stinger_physmap_t * map, char * string, uint64_t length);

uint64_t
stinger_physmap_get_mapping (stinger_physmap_t * map, char * string, uint64_t length);

int
stinger_physmap_get_key (stinger_physmap_t * map, char ** outbuffer, uint64_t * outbufferlength, uint64_t vertexID);

#define OF_VERTICES(array, count) vtx_filter = (array); vtx_filter_count = (count);  filt_mask |= 0x1;
#define OF_EDGE_TYPES(array, count) edge_type_filter = (array); edge_type_filter_count = (count); filt_mask |= 0x2;
#define OF_VERTEX_TYPES(array, count) vtx_type_filter = (array); vtx_type_filter_count = (count); filt_mask |= 0x4;
#define MODIFIED_BEFORE(time) modified_before = time; filt_mask |= 0x8;
#define MODIFIED_AFTER(time) modified_after = time; filt_mask |= 0x8;
#define CREATED_BEFORE(time) created_before = time; filt_mask |= 0x8;
#define CREATED_AFTER(time) created_after = time; filt_mask |= 0x8;

#define STINGER_TRAVERSE_EDGES(stinger, filter, code) do { \
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
        struct stinger_eb *  current_eb__ = vertices->vertices[(vtx_filter[k__])].edges;  \
        while(current_eb__ != ebpool_priv) {            \
          int64_t source__ = current_eb__->vertexID;      \
          int64_t type__ = current_eb__->etype;       \
          for(uint64_t j__ = 0; j__ < edge_type_filter_count; j__++) { \
            if(current_eb__->etype == edge_type_filter[j__]) {        \
              for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
                if(!stinger_eb_is_blank(current_eb__, i__)) {               \
                  struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
                  { code } \
                }               \
              }               \
              break; \
            }                 \
          } \
          current_eb__ = ebpool_priv+ (current_eb__->next);       \
        }                 \
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
        struct stinger_eb *  current_eb__ = vertices->vertices[(vtx_filter[k__])].edges;  \
        while(current_eb__ != ebpool_priv) {            \
          int64_t source__ = current_eb__->vertexID;      \
          int64_t type__ = current_eb__->etype;       \
          for(uint64_t j__ = 0; j__ < edge_type_filter_count; j__++) { \
            if(current_eb__->etype == edge_type_filter[j__]) {        \
              for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
                if(!stinger_eb_is_blank(current_eb__, i__)) {               \
                  struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
                  for(uint64_t p__ = 0; p__ < vtx_type_filter_count; p__++) { \
                    if(stinger_vtype((stinger), STINGER_EDGE_DEST) == vtx_type_filter[p__]) { \
                      { code }  \
                      break; \
                    } \
                  } \
                }               \
             }                \
          break; \
        }                 \
      } \
    current_eb__ = ebpool_priv+ (current_eb__->next);       \
  }                 \
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
  struct stinger_eb *  current_eb__ = vertices->vertices[(vtx_filter[k__])].edges;  \
  while(current_eb__ != ebpool_priv) {            \
    int64_t source__ = current_eb__->vertexID;      \
    int64_t type__ = current_eb__->etype;       \
    for(uint64_t j__ = 0; j__ < edge_type_filter_count; j__++) { \
      if(current_eb__->etype == edge_type_filter[j__]) {        \
        for(uint64_t i__ = 0; i__ < stinger_eb_high(current_eb__); i__++) { \
    if(!stinger_eb_is_blank(current_eb__, i__)) {               \
      struct stinger_edge * current_edge__ = current_eb__->edges + i__; \
      if(STINGER_EDGE_TIME_FIRST > created_after && STINGER_EDGE_TIME_FIRST < created_before && \
         STINGER_EDGE_TIME_RECENT > modified_after && STINGER_EDGE_TIME_RECENT < modified_before) { \
        { code } \
      } \
    }               \
        }               \
        break; \
      }                 \
    } \
    current_eb__ = ebpool_priv+ (current_eb__->next);       \
  }                 \
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
  struct stinger_eb *  current_eb__ = vertices->vertices[(vtx_filter[k__])].edges;  \
  while(current_eb__ != ebpool_priv) {            \
    int64_t source__ = current_eb__->vertexID;      \
    int64_t type__ = current_eb__->etype;       \
    for(uint64_t j__ = 0; j__ < edge_type_filter_count; j__++) { \
      if(current_eb__->etype == edge_type_filter[j__]) {        \
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
    }               \
        }               \
        break; \
      }                 \
    } \
    current_eb__ = ebpool_priv+ (current_eb__->next);       \
  }                 \
      } \
    } break; \
  } \
} while (0)

#endif  /*STINGER-DEPRECATED_H*/

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * END STINGER_PHYSMAP.H
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifdef __cplusplus
}
#undef restrict
#endif

#endif

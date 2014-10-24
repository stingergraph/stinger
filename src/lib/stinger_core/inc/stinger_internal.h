#if !defined(STINGER_INTERNAL_H_)
#define STINGER_INTERNAL_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#define MAP_STING(X) \
  stinger_vertices_t * vertices = (stinger_vertices_t *)((X)->storage); \
  stinger_physmap_t * physmap = (stinger_physmap_t *)((X)->storage + (X)->physmap_start); \
  stinger_names_t * etype_names = (stinger_names_t *)((X)->storage + (X)->etype_names_start); \
  stinger_names_t * vtype_names = (stinger_names_t *)((X)->storage + (X)->vtype_names_start); \
  uint8_t * _ETA = ((X)->storage + (X)->ETA_start); \
  struct stinger_ebpool * ebpool = (struct stinger_ebpool *)((X)->storage + (X)->ebpool_start);
	  
#define CONST_MAP_STING(X) \
  const stinger_vertices_t * vertices = (const stinger_vertices_t *)((X)->storage); \
  const stinger_physmap_t * physmap = (const stinger_physmap_t *)((X)->storage + (X)->physmap_start); \
  const stinger_names_t * etype_names = (const stinger_names_t *)((X)->storage + (X)->etype_names_start); \
  const stinger_names_t * vtype_names = (const stinger_names_t *)((X)->storage + (X)->vtype_names_start); \
  const uint8_t * _ETA = ((X)->storage + (X)->ETA_start); \
  const struct stinger_ebpool * ebpool = (const struct stinger_ebpool *)((X)->storage + (X)->ebpool_start);

#define ETA(X,Y) ((struct stinger_etype_array *)(_ETA + ((Y)*stinger_etype_array_size((X)->max_neblocks))))


#define STINGER_FORALL_EB_BEGIN(STINGER_,STINGER_SRCVTX_,STINGER_EBNM_)	\
  do {									\
    CONST_MAP_STING(STINGER_); \
    const struct stinger_eb * ebpool_priv = ebpool->ebpool;                    \
    const struct stinger * stinger__ = (STINGER_);			\
    const int64_t stinger_srcvtx__ = (STINGER_SRCVTX_);			\
    if (stinger_srcvtx__ >= 0) {					\
      const struct stinger_eb * restrict stinger_eb__;			\
      stinger_eb__ = ebpool_priv + stinger_vertex_edges_get(vertices, stinger_srcvtx__);	\
      while (stinger_eb__ != ebpool_priv) {						\
	const struct stinger_eb * restrict STINGER_EBNM_ = stinger_eb__; \
	do {								\

#define STINGER_FORALL_EB_END()						\
	} while (0);							\
	stinger_eb__ = stinger_next_eb (stinger__, stinger_eb__);	\
      }									\
    }									\
  } while (0)

#define STINGER_FORALL_EB_MODIFY_BEGIN(STINGER_,STINGER_SRCVTX_,STINGER_EBNM_) \
  do {									\
    MAP_STINGER(STINGER_); \
    const struct stinger_eb * ebpool_priv = ebpool->ebpool;                    \
    struct stinger * stinger__ = (STINGER_);				\
    int64_t stinger_srcvtx__ = (STINGER_SRCVTX_);			\
    if (stinger_srcvtx__ >= 0) {					\
      struct stinger_eb * stinger_eb__;					\
      stinger_eb__ = ebpool_priv + stinger_vertex_edges_get(vertices, stinger_srcvtx__);\
      while (stinger_eb__ != ebpool_priv) {						\
	struct stinger_eb * STINGER_EBNM_ = stinger_eb__;		\
	do {								\

#define STINGER_FORALL_EB_MODIFY_END()					\
	} while (0);							\
	stinger_eb__ = ebpool_priv + stinger_eb__->next;				\
      }									\
    }									\
  } while (0)

typedef uint64_t eb_index_t;

/**
* @brief A single edge in STINGER
*/
struct stinger_edge
{
  int64_t neighbor;	/**< The adjacent vertex ID */
  int64_t weight;	/**< The integer edge weight */
  int64_t timeFirst;	/**< First time stamp for this edge */
  int64_t timeRecent;	/**< Recent time stamp for this edge */
};

/**
* @brief An edge block in STINGER
*/
struct stinger_eb
{
  eb_index_t next;	    /**< Pointer to the next edge block */
  int64_t etype;	    /**< Edge type of this edge block */
  int64_t vertexID;	    /**< Source vertex ID associated with this edge block */
  int64_t numEdges;	    /**< Number of valid edges in the block */
  int64_t high;		    /**< High water mark */
  int64_t smallStamp;	    /**< Smallest timestamp in the block */
  int64_t largeStamp;	    /**< Largest timestamp in the block */
  int64_t cache_pad;	    /**< Does not do anything -- for performance reasons only */
  struct stinger_edge edges[STINGER_EDGEBLOCKSIZE]; /**< Array of edges */
};


/**
* @brief The edge type array
*/
struct stinger_etype_array
{
  int64_t length;     /**< Length of the edge type array */
  int64_t high;	      /**< High water mark in the edge type array */
  eb_index_t blocks[0];  /**< The edge type array itself, an array of edge block pointers */
};

struct stinger_ebpool {
  uint64_t ebpool_tail;
  uint8_t is_shared;
  struct stinger_eb ebpool[0];
};

/**
* @brief The STINGER data structure
*/
struct stinger
{
  uint64_t max_nv;
  uint64_t max_neblocks;
  uint64_t max_netypes;
  uint64_t max_nvtypes;

  /* uint64_t cur_max_nv; Someday... */
  uint64_t cur_ne;

  uint64_t vertices_start;
  uint64_t physmap_start;
  uint64_t etype_names_start;
  uint64_t vtype_names_start;
  uint64_t ETA_start;
  uint64_t ebpool_start;

  size_t length;
  uint8_t storage[0];
};

struct stinger_fragmentation_t {
  uint64_t num_empty_edges;
  uint64_t num_fragmented_blocks;
  uint64_t num_edges;
  uint64_t edge_blocks_in_use;
  double fill_percent;
};

struct curs
{
  eb_index_t eb, *loc;
};


static inline const struct stinger_eb *
stinger_next_eb (const struct stinger *G,
                 const struct stinger_eb *eb_);

int stinger_eb_high (const struct stinger_eb * eb_);
static inline int64_t stinger_eb_type (const struct stinger_eb * eb_);

int stinger_eb_is_blank (const struct stinger_eb * eb_, int k_);
int64_t stinger_eb_adjvtx (const struct stinger_eb *, int);
int64_t stinger_eb_weight (const struct stinger_eb *, int);
int64_t stinger_eb_ts (const struct stinger_eb *, int);
int64_t stinger_eb_first_ts (const struct stinger_eb *, int);

int64_t stinger_count_outdeg (struct stinger *G, int64_t v);

struct curs etype_begin (stinger_t * S, int64_t v, int etype);

void update_edge_data (struct stinger * S, struct stinger_eb *eb,
                  uint64_t index, int64_t neighbor, int64_t weight, int64_t ts);

void remove_edge (struct stinger * S, struct stinger_eb *eb, uint64_t index);

void new_ebs (struct stinger * S, eb_index_t *out, size_t neb, int64_t etype, int64_t from);

void push_ebs (struct stinger *G, size_t neb,
          eb_index_t * eb);

void stinger_fragmentation (struct stinger *S, uint64_t NV, struct stinger_fragmentation_t *frag);

void
new_blk_ebs (eb_index_t *out, const struct stinger * G,
             const int64_t nvtx, const size_t * blkoff,
             const int64_t etype);

#include "stinger_deprecated.h"

#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* STINGER_INTERNAL_H_ */

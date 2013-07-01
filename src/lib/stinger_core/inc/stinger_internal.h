#if !defined(STINGER_INTERNAL_H_)
#define STINGER_INTERNAL_H_

#define EBPOOL_SIZE (STINGER_MAX_LVERTICES*4)


#define STINGER_FORALL_EB_BEGIN(STINGER_,STINGER_SRCVTX_,STINGER_EBNM_)	\
  do {									\
    const struct stinger_eb * ebpool_priv = STINGER_->ebpool->ebpool;                    \
    const struct stinger * stinger__ = (STINGER_);			\
    const int64_t stinger_srcvtx__ = (STINGER_SRCVTX_);			\
    if (stinger_srcvtx__ >= 0) {					\
      const struct stinger_eb * restrict stinger_eb__;			\
      stinger_eb__ = ebpool_priv + stinger_vertex_edges_get(stinger__->vertices, stinger_srcvtx__);	\
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
    const struct stinger_eb * ebpool_priv = STINGER_->ebpool->ebpool;                    \
    struct stinger * stinger__ = (STINGER_);				\
    int64_t stinger_srcvtx__ = (STINGER_SRCVTX_);			\
    if (stinger_srcvtx__ >= 0) {					\
      struct stinger_eb * stinger_eb__;					\
      stinger_eb__ = ebpool_priv + stinger_vertex_edges_get(stinger__->vertices, stinger_srcvtx__);\
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
  eb_index_t next;  /**< Pointer to the next edge block */
  int64_t etype;	    /**< Edge type of this edge block */
  int64_t vertexID;	    /**< Source vertex ID associated with this edge block */
  int64_t numEdges;	    /**< Number of valid edges in the block */
  int64_t high;		    /**< High water mark */
  int64_t smallStamp;	    /**< Smallest timestamp in the block */
  int64_t largeStamp;	    /**< Largest timestamp in the block */
  struct stinger_edge edges[STINGER_EDGEBLOCKSIZE]; /**< Array of edges */
};


/**
* @brief The edge type array
*/
struct stinger_etype_array
{
  int64_t length;     /**< Length of the edge type array */
  int64_t high;	      /**< High water mark in the edge type array */
  eb_index_t blocks[EBPOOL_SIZE];  /**< The edge type array itself, an array of edge block pointers */
};

struct stinger_ebpool {
  struct stinger_eb ebpool[EBPOOL_SIZE];
  uint64_t ebpool_tail;
  uint8_t is_shared;
};

/**
* @brief The STINGER data structure
*/
struct stinger
{
  stinger_vertices_t  *	vertices; /**< Logical vertex array */
  stinger_names_t * etype_names;
  stinger_names_t * vtype_names;
  struct stinger_etype_array *ETA;  /**< One edge type array for each edge type, specified at compile time */
  struct stinger_ebpool * ebpool;
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

static inline int64_t stinger_nvtx_max (const struct stinger *);

int64_t
stinger_nvtx_max (const struct stinger *S_ /*UNUSED*/)
{
  return STINGER_MAX_LVERTICES;
}

/* internal filter and location data 
   intentionally hidden from users   */
struct stinger_iterator;
struct stinger_iterator_internal {
  struct    stinger * s;
  int64_t   flags;
  int64_t   modified_before;
  int64_t   modified_after;
  int64_t   created_before;
  int64_t   created_after;
  int64_t * vtx_filter;
  int64_t * edge_type_filter;
  int64_t * vtx_type_filter; 
  int64_t   vtx_filter_count;
  int64_t   edge_type_filter_count;
  int64_t   vtx_type_filter_count; 
  int	    vtx_type_filter_both;
  int	    vtx_filter_copy;
  int	    edge_type_filter_copy;
  int	    vtx_type_filter_copy;
  int 	    (*predicate)(struct stinger_iterator *);
  int	    active;

  int64_t edge_type_index;
  int64_t edge_block_index;

  int64_t vtx_index;

  struct stinger_eb * cur_eb;
  int64_t cur_edge;
};

static inline const struct stinger_eb *stinger_edgeblocks (const struct
							   stinger *,
							   int64_t);

static inline const struct stinger_eb *stinger_next_eb (const struct stinger
							*,
							const struct
							stinger_eb *);
int stinger_eb_high (const struct stinger_eb *);
static inline int64_t stinger_eb_type (const struct stinger_eb *);

int stinger_eb_is_blank (const struct stinger_eb *, int);
int64_t stinger_eb_adjvtx (const struct stinger_eb *, int);
int64_t stinger_eb_weight (const struct stinger_eb *, int);
int64_t stinger_eb_ts (const struct stinger_eb *, int);
int64_t stinger_eb_first_ts (const struct stinger_eb *, int);

int64_t stinger_count_outdeg (struct stinger *G, int64_t v);

struct curs etype_begin (stinger_t * S, stinger_vertices_t *vertices, int64_t v, int etype);

void update_edge_data (struct stinger * S, struct stinger_eb *eb,
                  uint64_t index, int64_t neighbor, int64_t weight, int64_t ts);

void remove_edge (struct stinger * S, struct stinger_eb *eb, uint64_t index);

void new_ebs (struct stinger * S, eb_index_t *out, size_t neb, int64_t etype, int64_t from);

void push_ebs (struct stinger *G, size_t neb,
          eb_index_t * eb);

const struct stinger_eb *
stinger_edgeblocks (const struct stinger *s_, int64_t i_);

const struct stinger_eb *
stinger_next_eb (const struct stinger *G /*UNUSED*/,
                 const struct stinger_eb *eb_);

int64_t
stinger_eb_type (const struct stinger_eb * eb_);

void stinger_fragmentation (struct stinger *S, uint64_t NV, struct stinger_fragmentation_t *frag);

void
new_blk_ebs (eb_index_t *out, const struct stinger * G,
             const int64_t nvtx, const size_t * blkoff,
             const int64_t etype);

#include "stinger_deprecated.h"

#endif /* STINGER_INTERNAL_H_ */

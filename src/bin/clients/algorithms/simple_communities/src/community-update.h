#if !defined(COMMUNITY_UPDATE_HEADER_)
#define COMMUNITY_UPDATE_HEADER_

#if defined(_OPENMP)
#include <omp.h>
#endif

struct community_state {
  int64_t graph_nv;
  size_t wslen;
  int64_t * ws;
  int64_t * restrict cmap;
  int64_t * restrict csize;
  int64_t * restrict mark;
  int64_t * restrict vlist;
  int64_t nvlist;
#if defined(_OPENMP)
  omp_lock_t * restrict lockspace;
#else
  void * lockspace;
#endif
  struct community_hist hist; /* = {{0.0}, 0, 0}; */
  struct el cg;

  int alg_score, alg_match;
  int64_t comm_limit;

  /* Stats... */
  int64_t nstep;
  int64_t n_nonsingletons;
  int64_t max_csize;
  double modularity;
};

void finalize_community_state (struct community_state * cstate);
int cstate_check (struct community_state *cstate);
void init_empty_community_state (struct community_state * cstate, const int64_t graph_nv, const int64_t ne_est);
double init_and_compute_community_state (struct community_state * cstate, struct el * g);
double init_and_read_community_state (struct community_state * cstate, int64_t graph_nv, const char *cg_name, const char *cmap_name);
void cstate_dump_cmap (struct community_state * cstate, long which, long num);
double cstate_update (struct community_state * cstate, const struct stinger * S);

void cstate_preproc (struct community_state * restrict,
                     const struct stinger *,
                     const int64_t, const int64_t * restrict,
                     const int64_t, const int64_t * restrict);

void cstate_preproc_acts (struct community_state * restrict,
                          const struct stinger *,
                          const int64_t, const int64_t * restrict);

#if !defined(INSQUEUE_SIZE)
#if defined(__MTA__)
/* Just to reduce hot spotting */
#define INSQUEUE_SIZE 64
#else
/* Cache-based architectures should shoot for L1. */
/* each edge is 3*8=24 bytes, L1 is... 64kb.  nedge: 1024? 512? */
#define INSQUEUE_SIZE 1024
#endif
#endif
struct insqueue {
  int n;
#if !defined(NO_QUEUE)
  intvtx_t q[3 * INSQUEUE_SIZE];
#endif
};

void qflush (struct insqueue * restrict q, struct el * restrict g);
void enqueue (struct insqueue * restrict q,
              intvtx_t i, intvtx_t j, intvtx_t w,
              struct el * restrict g);

#endif /* COMMUNITY_UPDATE_HEADER_ */

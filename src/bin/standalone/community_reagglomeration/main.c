/* -*- mode: C; fill-column: 70; -*- */
#define _XOPEN_SOURCE 600
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include "stinger_core/stinger_atomics.h"
#include "stinger_utils/stinger_utils.h"
#if !defined(TRIVIAL_INSREM)
#include "stinger_core/stinger_deprecated.h"
#endif
#include "stinger_core/stinger.h"
#include "stinger_utils/timer.h"
#include "stinger_core/xmalloc.h"

#include "compat.h"
#include "sorts.h"
#include "graph-el.h"
#include "community.h"
#include "community-update.h"

#include "xmt-luc.h"

#include "bsutil.h"

#if !defined(CCAT)
#define CCAT(A,B) A##B
#endif
#if !defined(CONCAT)
#define CONCAT(A,B) CCAT(A,B)
#endif

#define ACTI(k) (action[2*(k)])
#define ACTJ(k) (action[2*(k)+1])

static int64_t nv, ne, naction;
static int64_t * restrict off;
static int64_t * restrict from;
static int64_t * restrict ind;
static int64_t * restrict weight;
static int64_t * restrict action;

/* handles for I/O memory */
static int64_t * restrict graphmem;
static int64_t * restrict actionmem;

static char * initial_graph_name = INITIAL_GRAPH_NAME_DEFAULT;
static char * action_stream_name = ACTION_STREAM_NAME_DEFAULT;

static int64_t comm_limit = INT64_MAX;

static long batch_size = BATCH_SIZE_DEFAULT;
static long nbatch = NBATCH_DEFAULT;

static struct stinger * S;

static int64_t ntrace = 0;
static int64_t * ncomm_trace;
static int64_t * ncel_trace;
static int64_t * nactvtx_trace;
static int64_t * nactel_trace;
static int64_t * nrestep_trace;
static double * ds_time_trace;
static double * update_time_trace;
static int64_t * max_comm_trace;

static struct el stinger_to_el (const struct stinger *, const int64_t, const int64_t);

int
main (const int argc, char *argv[])
{
  struct community_state cstate;

  parse_args (argc, argv, &initial_graph_name, &action_stream_name, &batch_size, &nbatch);

  STATS_INIT();

  load_graph_and_action_stream (initial_graph_name, &nv, &ne, (int64_t**)&off,
	      (int64_t**)&ind, (int64_t**)&weight, (int64_t**)&graphmem,
	      action_stream_name, &naction, (int64_t**)&action, (int64_t**)&actionmem);

  print_initial_graph_stats (nv, ne, batch_size, nbatch, naction);
  BATCH_SIZE_CHECK();

  if (nv >= STINGER_MAX_LVERTICES) {
    fprintf (stderr, "Increase STINGER_MAX_LVERTICES to at least %ld\n", (long)nv);
  }

  ncomm_trace = xmalloc (nbatch * sizeof(*ncomm_trace));
  ncel_trace = xmalloc (nbatch * sizeof(*ncel_trace));
  nactvtx_trace = xmalloc (nbatch * sizeof(*nactvtx_trace));
  nactel_trace = xmalloc (nbatch * sizeof(*nactel_trace));
  nrestep_trace = xmalloc (nbatch * sizeof(*nrestep_trace));
  ds_time_trace = xmalloc (nbatch * sizeof(*ds_time_trace));
  update_time_trace = xmalloc (nbatch * sizeof(*update_time_trace));
  max_comm_trace = xmalloc (nbatch * sizeof(*max_comm_trace));

#if !defined(TRIVIAL_INSREM)
  int64_t * act = xmalloc (2 * 2 * batch_size * sizeof(*act));
  int64_t * insoff = xmalloc ((2 * 2 * batch_size + 1) * sizeof(*insoff));
  int64_t * deloff = xmalloc ((2 * 2 * batch_size + 1) * sizeof(*deloff));
#endif

  /* Convert to STINGER */
  tic ();
  S = stinger_new ();
  stinger_set_initial_edges (S, nv, 0, off, ind, weight, NULL, NULL, 0);
  PRINT_STAT_DOUBLE ("time_stinger", toc ());
  fflush(stdout);

  free(graphmem);

  tic ();
  uint32_t errorCode = stinger_consistency_check (S, nv);
  double time_check = toc ();
  printf("\n\t\"error_code\": 0x%lx", (long)errorCode);
  PRINT_STAT_DOUBLE ("time_check", time_check);

  if (!getenv ("READ_CGRAPH")) {
    tic ();
    struct el el = stinger_to_el (S, nv, ne);
    PRINT_STAT_DOUBLE ("time_stinger_to_el", toc ());
    fflush(stdout);

    double time_init_comm;
    time_init_comm = init_and_compute_community_state (&cstate, &el);
    PRINT_STAT_DOUBLE ("time_init_comm", time_init_comm);
  } else {
    if (!getenv ("READ_CBIN")) {
      fprintf (stderr, "Must provide both READ_CBIN and READ_CGRAPH.\n");
      return EXIT_FAILURE;
    }
    double time_read_init_comm;
    time_read_init_comm = init_and_read_community_state (&cstate, nv,
                                                         getenv ("READ_CGRAPH"),
                                                         getenv ("READ_CBIN"));
    PRINT_STAT_DOUBLE ("time_read_init_comm", time_read_init_comm);
  }

  PRINT_STAT_INT64 ("ncomm_init", cstate.cg.nv);
  PRINT_STAT_INT64 ("max_comm_init", cstate.max_csize);

  /* Updates */
  fflush (stdout);

  if (getenv("DUMP_CMAP"))
    cstate_dump_cmap (&cstate, 0, batch_size);

#if defined(_OPENMP)
  int max_threads = omp_get_max_threads ();
  int nt = max_threads;
  int set_threads = getenv ("NT") != NULL;
  if (set_threads) {
    nt = strtol (getenv ("NT"), NULL, 10);
    omp_set_num_threads (nt);
  }
  PRINT_STAT_INT64 ("max_threads", (int64_t)max_threads);
  PRINT_STAT_INT64 ("num_threads", (int64_t)nt);
#endif

  for (int64_t actno = 0; actno < nbatch * batch_size; actno += batch_size)
  {
    printf ("updating [%ld, %ld)\n", (long)actno, (long)(actno+batch_size));
    fflush (stdout);

    tic();

    const int64_t endact = (actno + batch_size > naction ? naction : actno + batch_size);
    int64_t *actionStream = &action[2*actno];
    int64_t numActions = endact - actno;

#if !defined(NDEBUG)
    assert (cstate_check (&cstate));
#endif

    cstate_preproc_acts (&cstate, S, numActions, actionStream);

    /* Update STINGER structure */
#if !defined(TRIVIAL_INSREM)
    MTA("mta assert parallel")
    MTA("mta block dynamic schedule")
    OMP("omp parallel for")
    for(uint64_t k = 0; k < numActions; k++) {
      const int64_t i = actionStream[2 * k];
      const int64_t j = actionStream[2 * k + 1];

      if (i != j && i < 0) {
        stinger_remove_edge(S, 0, ~i, ~j);
        stinger_remove_edge(S, 0, ~j, ~i);
      }

      if (i != j && i >= 0) {
        stinger_insert_edge (S, 0, i, j, 1, actno+2);
        stinger_insert_edge (S, 0, j, i, 1, actno+2);
      }
    }
#else
    int64_t N = stinger_sort_actions (numActions, actionStream, insoff, deloff, act);
    stinger_remove_and_insert_batch (S, 0, actno+2, N, insoff, deloff, act);
#endif

    ds_time_trace[ntrace] = toc ();
    printf ("\tinserted   %g\n", ds_time_trace[ntrace]); fflush (stdout);

#if !defined(NDEBUG)
    assert (cstate_check (&cstate));
#endif

    update_time_trace[ntrace] = cstate_update (&cstate, S);
    nrestep_trace[ntrace] = cstate.nstep;

    ncomm_trace[ntrace] = cstate.cg.nv;
    ncel_trace[ntrace] = cstate.cg.ne;
    //printf ("ncomm[%ld]: %ld\n", (long)ntrace, (long)el.nv);
    max_comm_trace[ntrace] = cstate.max_csize;

    if (getenv("DUMP_CMAP"))
      cstate_dump_cmap (&cstate, actno, batch_size);

    ntrace++;
  } /* End of batch */

#if defined(_OPENMP)
  if (set_threads) {
    omp_set_num_threads (max_threads);
  }
#endif

  /* Print the times */
  double time_updates = 0;
  for (int64_t k = 0; k < ntrace; k++) {
    time_updates += update_time_trace[k];
  }
  PRINT_STAT_DOUBLE ("time_updates", time_updates);
  PRINT_STAT_DOUBLE ("updates_per_sec", (ntrace * batch_size) / time_updates); 

  tic ();
  errorCode = stinger_consistency_check (S, nv);
  time_check = toc ();
  printf("\n\t\"error_code\": 0x%lx", (long)errorCode);
  PRINT_STAT_DOUBLE ("time_check", time_check);
  STATS_END();

  printf ("update_time_trace:");
  for (int64_t k = 0; k < ntrace; ++k)
    printf (" %g", update_time_trace[k]);
  printf ("\n");
  printf ("ncomm_trace:");
  for (int64_t k = 0; k < ntrace; ++k)
    printf (" %ld", (long)ncomm_trace[k]);
  printf ("\n");
  printf ("max_comm_trace:");
  for (int64_t k = 0; k < ntrace; ++k)
    printf (" %ld", (long)max_comm_trace[k]);
  printf ("\n");
  printf ("ncel_trace:");
  for (int64_t k = 0; k < ntrace; ++k)
    printf (" %ld", (long)ncel_trace[k]);
  printf ("\n");
  printf ("nactvtx_trace:");
  for (int64_t k = 0; k < ntrace; ++k)
    printf (" %ld", (long)nactvtx_trace[k]);
  printf ("\n");
  printf ("nactel_trace:");
  for (int64_t k = 0; k < ntrace; ++k)
    printf (" %ld", (long)nactel_trace[k]);
  printf ("\n");
  printf ("nrestep_trace:");
  for (int64_t k = 0; k < ntrace; ++k)
    printf (" %ld", (long)nrestep_trace[k]);
  printf ("\n");

  finalize_community_state (&cstate);

  stinger_free_all (S);
}

struct el
stinger_to_el (const struct stinger * S, const int64_t nv, const int64_t ne)
{
  struct el el;

  el = alloc_graph (nv, ne);
  memset (el.d, 0, nv * sizeof (*el.d));
  el.ne = 0;
  assert (el.el);
  OMP("omp parallel") {
#if !defined(__MTA__)
    struct insqueue q;
    q.n = 0;
#endif

    OMP("omp for") MTA("mta assert nodep")
      for (int64_t i = 0; i < nv; ++i) {
#if defined(__MTA__)
        struct insqueue q;
        q.n = 0;
#endif

        STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
          const int64_t j = STINGER_EDGE_DEST;
          const int64_t w = STINGER_EDGE_WEIGHT;
          assert (el.ne + q.n < el.ne_orig);
          if ((i ^ j) & 0x01)
            enqueue (&q, i, j, w, &el);
          else if (i == j)
            el.d[i] += w;
        } STINGER_FORALL_EDGES_OF_VTX_END();
#if defined(__MTA__)
        qflush (&q, &el);
#endif
      }

#if !defined(__MTA__)
    qflush (&q, &el);
#endif
  }

  return el;
}

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"

#include "defs.h"
#include "graph-el.h"
#include "community.h"
#include "community-update.h"

int initial_compute = 0;
int run_update_alg = 1;
int64_t iter = 0;
double comm_time;
int64_t nv;
struct community_state cstate;
int64_t * restrict cmap;

double init_cstate_from_stinger (struct community_state *, const struct stinger *);
double cstate_preproc_alg (struct community_state * restrict cstate,
                          const stinger_registered_alg * alg);

int
main(int argc, char *argv[])
{
  if (getenv ("COMPUTE"))
    initial_compute = 1;
  for (int k = 1; k < argc; ++k) {
    if (0 == strcmp(argv[k], "--compute"))
      initial_compute = 1;
    else if (0 == strcmp(argv[k], "--no-update"))
      run_update_alg = 0;
    else if (0 == strcmp(argv[k], "--help") || 0 == strcmp(argv[1], "-h")) {
      fprintf (stderr, "Pass --compute or set COMPUTE env. var to compute\n"
              "an initial community map, --no-update to re-run static alg.");
    }
  }

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_registered_alg * alg =
    stinger_register_alg(
      .name="simple_communities",
      .data_per_vertex=sizeof(int64_t),
      .data_description="l community_label",
      .host="localhost",
    );

  if(!alg) {
    LOG_E("Registering algorithm failed.  Exiting");
    return -1;
  }

  nv = (stinger_max_active_vertex(alg->stinger))?stinger_max_active_vertex(alg->stinger) + 1:0;
  cmap = (int64_t *)alg->alg_data;
#define UPDATE_EXTERNAL_CMAP(CS) do {                   \
    OMP("omp parallel for")                             \
      for (intvtx_t k = 0; k < CS.graph_nv; ++k)        \
        cmap[k] = CS.cmap[k];                           \
  } while (0)

  if (run_update_alg) {
    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
     * Initial static computation
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    stinger_alg_begin_init(alg); {
      const struct stinger * S = alg->stinger;
      if (initial_compute)
        comm_time = init_cstate_from_stinger (&cstate, S);
      else {
        comm_time = 0;
        init_empty_community_state (&cstate, nv, (1+stinger_total_edges(S))/2);
      }
      UPDATE_EXTERNAL_CMAP(cstate);
    } stinger_alg_end_init(alg);

    LOG_V_A("simple_communities: init comm time = %g\n", comm_time);

    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
     * Streaming Phase
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    while(alg->enabled) {
      ++iter;
      /* Pre processing */
      if(stinger_alg_begin_pre(alg)) {
        /* Must be here to find the removal weights. */
        comm_time = cstate_preproc_alg (&cstate, alg);
        stinger_alg_end_pre(alg);
        LOG_V_A("simple_communities: preproc time %ld = %g", (long)iter, comm_time);
      }

      /* Post processing */
      if(stinger_alg_begin_post(alg)) {
        comm_time = cstate_update (&cstate, alg->stinger);
        UPDATE_EXTERNAL_CMAP(cstate);
        stinger_alg_end_post(alg);
        LOG_V_A("simple_communities: proc time %ld = %g", (long)iter, comm_time);
        LOG_V_A("simple_communities: ncomm %ld = %ld", (long)iter, (long)cstate.cg.nv);
      }
    }
  } else { /* Re-run static community detection */
    /* This whole clause is a nasty hack... */
    stinger_alg_begin_init(alg); {
      const struct stinger * S = alg->stinger;
      comm_time = init_cstate_from_stinger (&cstate, S);
      UPDATE_EXTERNAL_CMAP(cstate);
      finalize_community_state (&cstate);
    } stinger_alg_end_init(alg);

    LOG_V_A("simple_communities no-update: init comm time = %g", comm_time);

    while (alg->enabled) {
      ++iter;
      if(stinger_alg_begin_pre(alg)) {
        /* Nothing... */
        stinger_alg_end_pre(alg);
        LOG_V_A("simple_communities no-update: preproc time %ld = %g", (long)iter, 0.0);
      }

      if (stinger_alg_begin_post(alg)) {
        const struct stinger * S = alg->stinger;
        struct community_state cstate_iter;
        intvtx_t ncomm;
        comm_time = init_cstate_from_stinger (&cstate_iter, S);
        UPDATE_EXTERNAL_CMAP(cstate_iter);
        stinger_alg_end_post(alg);
        ncomm = cstate_iter.cg.nv;
        finalize_community_state (&cstate_iter);
        LOG_V_A("simple_communities no-update: proc time %ld = %g", (long)iter, comm_time);
        LOG_V_A("simple_communities: ncomm %ld = %ld", (long)iter, (long)ncomm);
      }
    }
  }

  LOG_I("Algorithm complete... shutting down");
  finalize_community_state (&cstate);
  xfree(alg);
}

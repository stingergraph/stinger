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
  if (argc > 1) {
    if (0 == strcmp(argv[1], "--compute"))
      initial_compute = 1;
    else if (0 == strcmp(argv[1], "--help") || 0 == strcmp(argv[1], "-h")) {
      fprintf (stderr, "Pass --compute or set COMPUTE env. var to compute\n"
              "an initial community map.");
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

  nv = stinger_max_active_vertex(alg->stinger)+1;
  cmap = (int64_t *)alg->alg_data;

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {
    const struct stinger * S = alg->stinger;
    init_empty_community_state (&cstate, nv, 2*nv);
    free (cstate.cmap);
    cstate.cmap = cmap;
    if (initial_compute)
      init_cstate_from_stinger (&cstate, S);
    else
      init_empty_community_state (&cstate, nv, (1+stinger_total_edges(S))/2);
  } stinger_alg_end_init(alg);

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Streaming Phase
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  while(alg->enabled) {
    /* Pre processing */
    if(stinger_alg_begin_pre(alg)) {
      /* Must be here to find the removal weights. */
      cstate_preproc_alg (&cstate, alg);
      stinger_alg_end_pre(alg);
    }

    /* Post processing */
    if(stinger_alg_begin_post(alg)) {
      cstate_update (&cstate, alg->stinger);
      stinger_alg_end_post(alg);
    }
  }

  LOG_I("Algorithm complete... shutting down");
  finalize_community_state (&cstate);
}

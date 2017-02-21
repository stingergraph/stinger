#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_core/stinger_error.h"
#include "stinger_alg/streaming_connected_components.h"


int
main (int argc, char *argv[])
{
	fflush(stdout);
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_registered_alg * alg =
    stinger_register_alg(
      .name="streaming_connected_components",
      .data_per_vertex=sizeof(int64_t)*2,
      .data_description="ll component_label component_size",
      .host="localhost",
    );

  if (!alg) {
    LOG_E("Registering algorithm failed.  Exiting");
    return -1;
  }

  bzero(alg->alg_data, sizeof(int64_t) * 2 * alg->stinger->max_nv);
  int64_t * components = (int64_t *)alg->alg_data;
  int64_t * component_size  = components + alg->stinger->max_nv;


	stinger_scc_internal scc_internal;
	stinger_connected_components_stats stats;

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {
    /* find the type */

	stinger_scc_initialize_internals(alg->stinger,alg->stinger->max_nv,&scc_internal,4);
	stinger_scc_reset_stats(&stats);
  } stinger_alg_end_init(alg);

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Streaming Phase
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  while(alg->enabled) {
    /* Pre processing */
    if(stinger_alg_begin_pre(alg)) {
      /* nothing to do */
      stinger_alg_end_pre(alg);
    }

    /* Post processing */
    if(stinger_alg_begin_post(alg)) {
	  stinger_scc_insertion(alg->stinger,alg->stinger->max_nv,scc_internal,&stats,alg->insertions,alg->num_insertions);
	  stinger_scc_deletion(alg->stinger,alg->stinger->max_nv,scc_internal,&stats,alg->deletions,alg->num_deletions);

	  stinger_scc_copy_component_array(scc_internal,alg->alg_data);

      stinger_alg_end_post(alg);
    }
  }
	// stinger_scc_release_internals(&scc_internal);

  LOG_I("Algorithm complete... shutting down");
}

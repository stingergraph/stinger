#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_core/stinger_error.h"
#include "stinger_alg/weakly_connected_components.h"


int
main (int argc, char *argv[])
{
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_registered_alg * alg =
    stinger_register_alg(
      .name="weakly_connected_components",
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

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {
    /* find the type */
    int64_t type = -1;
    if (argc > 1) {
      type = stinger_etype_names_lookup_type(alg->stinger, argv[1]);
    }
    /* calculate connected components */
    parallel_shiloach_vishkin_components_of_type(alg->stinger, components, type);
    /* summarize the data */
    compute_component_sizes (alg->stinger, components, component_size);
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
      /* find the type */
      int64_t type = -1;
      if (argc > 1) {
	       type = stinger_etype_names_lookup_type(alg->stinger, argv[1]);
      }
      parallel_shiloach_vishkin_components_of_type(alg->stinger, components, type);
      compute_component_sizes (alg->stinger, components, component_size);
      stinger_alg_end_post(alg);
    }
  }

  LOG_I("Algorithm complete... shutting down");
  xfree(alg);
}

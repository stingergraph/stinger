#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_alg/kcore.h"


int
main(int argc, char *argv[])
{
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_registered_alg * alg =
    stinger_register_alg(
      .name="kcore",
      .data_per_vertex=2*sizeof(int64_t),
      .data_description="ll kcore size\n"
      "  kcore - a label per vertex, the k of the largest core in which that vertex belongs\n"
      "  count - the number of neighbors also in this large core\n"
      "This algorithm gives an indication of the largest k-core to which each\n"
      "vertex belongs by counting the number of neighboring vertices in the k-1 core and its neighbors'\n"
      "counts of how many neighbors they have in a k-1 core. If their are enough, the vertex upgrades\n"
      "itself to k.",
      .host="localhost",
    );

  if(!alg) {
    LOG_E("Registering algorithm failed.  Exiting");
    return -1;
  }

  int64_t * kcore = (int64_t *)alg->alg_data;
  int64_t * count = ((int64_t *)alg->alg_data) + alg->stinger->max_nv;
  int64_t k;

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {
    LOG_I("Kcore init starting");
    kcore_find(alg->stinger, kcore, count, alg->stinger->max_nv, &k);
    LOG_I_A("Kcore init finished. Largest core is %ld", (long)k);
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
      LOG_I("Kcore post starting");
      kcore_find(alg->stinger, kcore, count, alg->stinger->max_nv, &k);
      LOG_I_A("Kcore post finished. Largest core is %ld", (long)k);
      stinger_alg_end_post(alg);
    }
  }

  LOG_I("Algorithm complete... shutting down");
  xfree(alg);
}

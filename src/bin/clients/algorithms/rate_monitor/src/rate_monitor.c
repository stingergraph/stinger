#include <stdint.h>
#include <strings.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"

void
update_rates(stinger_registered_alg * a, int64_t nv, double * vel, double * accel, double vel_keep, double accel_keep)
{
  int64_t * count = calloc(nv, sizeof(int64_t));

  OMP("omp parallel for")
  for(int64_t i = 0; i < a->num_insertions; i++) {
    stinger_int64_fetch_add(count + a->insertions[i].source,1);
    stinger_int64_fetch_add(count + a->insertions[i].destination,1);
  }

  OMP("omp parallel for")
  for(int64_t i = 0; i < a->num_deletions; i++) {
    stinger_int64_fetch_add(count + a->deletions[i].source,1);
    stinger_int64_fetch_add(count + a->deletions[i].destination,1);
  }

  OMP("omp parallel for")
  for(int64_t v = 0; v < nv; v++) {
    accel[v] = (accel_keep * accel[v]) + (1 - accel_keep) * (count[v] - vel[v]);
    vel[v] = (vel_keep * vel[v]) + (1 - vel_keep) * count[v];
  }


  free(count);
}

int
main(int argc, char *argv[]) 
{
  double vel_keep = 0.333;
  double accel_keep = 0.333;

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_registered_alg * alg = 
    stinger_register_alg(
      .name="rate_monitor",
      .data_per_vertex=sizeof(double)*2,
      .data_description="dd wgtd_edge_vel wgtd_edge_accel",
      .host="localhost",
    );

  if(!alg) {
    LOG_E("Registering algorithm failed.  Exiting");
    return -1;
  }

  bzero(alg->alg_data, sizeof(double) * 2 * alg->stinger->max_nv);
  double * vel	  = (double *)alg->alg_data;
  double * accel  = vel + alg->stinger->max_nv;
  double time;

  
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {
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
      time = timer();
      update_rates(alg, stinger_max_active_vertex(alg->stinger)+1, vel, accel, vel_keep, accel_keep);
      time = timer() - time;
      LOG_I_A("Update time : %20.15e sec", time);
      stinger_alg_end_post(alg);
    }
  }

  LOG_I("Algorithm complete... shutting down");
}

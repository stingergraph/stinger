#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_core/stinger_error.h"


/*
 * Compute number of users per component
 */
int64_t
get_number_of_users_per_component (struct stinger * S,
				  int64_t * component_map,
				  double * influence_score,
				  int64_t * total_users, 
				  double * total_influence)
{
  int64_t nv = S->max_nv;
  int64_t tweets_etype = stinger_etype_names_lookup_type(S, "tweets");
  int64_t user_vtype = stinger_vtype_names_lookup_type(S, "user");
  int64_t tweet_vtype = stinger_vtype_names_lookup_type(S, "tweet");
  if (user_vtype < 0) {
    return -1;
  }

  OMP ("omp parallel for")
  for (uint64_t i = 0; i < nv; i++) {
    total_users[i] = 0;
  }

  OMP ("omp parallel for")
  for (uint64_t i = 0; i < nv; i++) {
    total_influence[i] = 0;
  }

  OMP ("omp parallel for")
  for (uint64_t i = 0; i < nv; i++) {
    if (stinger_vtype_get(S, i) == user_vtype) {
      /* track the conversation IDs */
      int64_t * user_to_conversation = xmalloc (nv * sizeof(int64_t));
      for (uint64_t j = 0; j < nv; j++) {
	user_to_conversation[j] = 0;
      }
     
      /* iterate through all the conversation IDs we are involved in and mark them */
      STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(S, tweets_etype, i) {
	int64_t c_num = component_map[STINGER_EDGE_DEST];
	user_to_conversation[c_num] = 1;
      } STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_END();

      double my_influence = influence_score[i];
      /* iterate all the marks and increment the user count */
      for (uint64_t j = 0; j < nv; j++) {
	if (user_to_conversation[j]) {
	  /* user count */
	  stinger_int64_fetch_add(&total_users[j], 1);

	  /* influence count */
	  OMP("omp critical")
	  total_influence[j] += my_influence;
	}
      }

      free(user_to_conversation);

    }
  }

  return 0;
}


int
main (int argc, char *argv[]) 
{
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  size_t local_data_sz = sizeof(int64_t) + sizeof(double);

  stinger_registered_alg * alg = 
    stinger_register_alg(
      .name="influence_aggregator",
      .data_per_vertex=local_data_sz,
      .data_description="ld total_users total_influence",
      .host="localhost",
      .num_dependencies=2,
      .dependencies=(char *[]){"weakly_connected_components", "betweenness_centrality"}
    );

  if (!alg) {
    LOG_E("Registering algorithm failed.  Exiting");
    return -1;
  }

  /* our local published working storage */
  bzero(alg->alg_data, local_data_sz * alg->stinger->max_nv);
  int64_t * total_users = (int64_t *)alg->alg_data;
  double * total_influence  = (double *) (total_users + alg->stinger->max_nv);

  /* shared read-only from connected components */
  int64_t * component_map = (int64_t *)alg->dep_data[0];
  int64_t * component_size  = component_map + alg->stinger->max_nv;
 
  /* shared read-only from betweenness centrality */
  double * bc = (double *)alg->dep_data[1];
  int64_t * times_found = (int64_t *) (bc + alg->stinger->max_nv);
  
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {
    /* calculate number of users in each component */
    get_number_of_users_per_component (alg->stinger, component_map, bc, total_users, total_influence);
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
      /* calculate number of users in each component */
      get_number_of_users_per_component (alg->stinger, component_map, bc, total_users, total_influence);
      LOG_D_A("component_map[0]: %ld", component_map[0]);
      LOG_D_A("component_size[0]: %ld", component_size[0]);
      stinger_alg_end_post(alg);
    }
  }

  LOG_I("Algorithm complete... shutting down");
}

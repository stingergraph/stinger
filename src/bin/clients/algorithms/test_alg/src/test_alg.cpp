extern "C" {
  #include "stinger_core/stinger.h"
  #include "stinger_core/stinger_error.h"
  #include "stinger_net/stinger_alg.h"
}

int
main(int argc, char *argv[])
{
  if(argc < 4) {
    printf("too few args");
    return -1;
  }

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_register_alg_params register_params;
  memset(&register_params, 0, sizeof(stinger_register_alg_params));

  register_params.name=argv[1];
  register_params.data_per_vertex=atol(argv[2]);
  register_params.data_description=argv[3];
  register_params.host="localhost";

  if(argc > 4) {
    register_params.num_dependencies = argc - 4;
    register_params.dependencies = argv + 4;
  }

  stinger_registered_alg * alg = stinger_register_alg_impl(register_params);

  if(!alg) {
    LOG_E("Registering algorithm failed.  Exiting");
    return -1;
  }


  /* Print dependency storage information */
  for(int a = 0; a < argc - 4; a++) {
    printf("Alg %s\nLoc %s\nDesc %s\nPtr %p\nData Per %ld\n",
      alg->dep_name[a], alg->dep_location[a], alg->dep_description[a], 
      alg->dep_data[a], alg->dep_data_per_vertex[a]);
  }

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {

    LOG_I_A("Initializing algorithm... consistency check: %ld",
      (long) stinger_consistency_check(alg->stinger, STINGER_MAX_LVERTICES));

  } stinger_alg_end_init(alg);

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Streaming Phase
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  while(alg->enabled) {

    /* Preprocessing */
    if(stinger_alg_begin_pre(alg)) {

      LOG_I_A("Preprocessing algorithm... consistency check: %ld",
	(long) stinger_consistency_check(alg->stinger, STINGER_MAX_LVERTICES));

      LOG_I_A("Preprocessing algorithm... num_insertions: %ld",
	alg->num_insertions);
      for(int64_t i = 0; i < alg->num_insertions; i++) {
	LOG_I_A("\tINS: TYPE %s %ld FROM %s %ld TO %s %ld WEIGHT %ld TIME %ld", 
	  alg->insertions[i].type_str,
	  alg->insertions[i].type,
	  alg->insertions[i].source_str,
	  alg->insertions[i].source,
	  alg->insertions[i].destination_str,
	  alg->insertions[i].destination,
	  alg->insertions[i].weight,
	  alg->insertions[i].time);
      }

      LOG_I_A("Preprocessing algorithm... num_deletions: %ld",
	alg->num_deletions);
      for(int64_t i = 0; i < alg->num_deletions; i++) {
	LOG_I_A("\tDEL: TYPE %s %ld FROM %s %ld TO %s %ld",
	  alg->deletions[i].type_str,
	  alg->deletions[i].type,
	  alg->deletions[i].source_str,
	  alg->deletions[i].source,
	  alg->deletions[i].destination_str,
	  alg->deletions[i].destination);
      }

      stinger_alg_end_pre(alg);
    }

    /* Post processing */
    if(stinger_alg_begin_post(alg)) {

      LOG_I_A("Postprocessing algorithm... consistency check: %ld",
	(long) stinger_consistency_check(alg->stinger, STINGER_MAX_LVERTICES));

      LOG_I_A("Postprocessing algorithm... num_insertions: %ld",
	alg->num_insertions);
      for(int64_t i = 0; i < alg->num_insertions; i++) {
	LOG_I_A("\tINS: TYPE %s %ld FROM %s %ld TO %s %ld WEIGHT %ld TIME %ld", 
	  alg->insertions[i].type_str,
	  alg->insertions[i].type,
	  alg->insertions[i].source_str,
	  alg->insertions[i].source,
	  alg->insertions[i].destination_str,
	  alg->insertions[i].destination,
	  alg->insertions[i].weight,
	  alg->insertions[i].time);
      }

      LOG_I_A("Postprocessing algorithm... num_deletions: %ld",
	alg->num_deletions);
      for(int64_t i = 0; i < alg->num_deletions; i++) {
	LOG_I_A("\tDEL: TYPE %s %ld FROM %s %ld TO %s %ld",
	  alg->deletions[i].type_str,
	  alg->deletions[i].type,
	  alg->deletions[i].source_str,
	  alg->deletions[i].source,
	  alg->deletions[i].destination_str,
	  alg->deletions[i].destination);
      }

      stinger_alg_end_post(alg);
    }
  }

  LOG_I("Algorithm complete... shutting down");
}

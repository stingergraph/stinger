#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"

#include "stinger_alg/pagerank.h"

int
main(int argc, char *argv[])
{
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  char name[1024];
  char type_str[1024];

  int type_specified = 0;
  int directed = 0;

  double epsilon = EPSILON_DEFAULT;
  double dampingfactor = DAMPINGFACTOR_DEFAULT;
  int64_t maxiter = MAXITER_DEFAULT;
  
  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "t:e:f:i:d?h"))) {
    switch(opt) {
      case 't': {
        sprintf(name, "pagerank_%s", optarg);
        strcpy(type_str,optarg);
        type_specified = 1;
      } break;
      case 'd': {
        directed = 1;
      } break;
      case 'e': {
        epsilon = atof(optarg);
      } break;
      case 'f': {
        dampingfactor = atof(optarg);
      } break;
      case 'i': {
        maxiter = atol(optarg);
      } break;
      default:
        printf("Unknown option '%c'\n", opt);
      case '?':
      case 'h': {
        printf(
                "PageRank\n"
                        "==================================\n"
                        "\n"
                        "  -t <str>  Specify an edge type to run page rank over\n"
                        "  -d        Use a PageRank that is safe on directed graphs\n"
                        "  -e        Set PageRank Epsilon (default: %0.1e)\n"
                        "  -f        Set PageRank Damping Factor (default: %lf)\n"
                        "  -i        Set PageRank Max Iterations (default: %ld)\n"
                        "\n",EPSILON_DEFAULT,DAMPINGFACTOR_DEFAULT,MAXITER_DEFAULT);
        return(opt);
      }
    }
  }

  stinger_registered_alg * alg = 
    stinger_register_alg(
      .name=type_specified ? name : "pagerank",
      .data_per_vertex=sizeof(double),
      .data_description="d pagerank",
      .host="localhost",
    );

  if(!alg) {
    LOG_E("Registering algorithm failed.  Exiting");
    return -1;
  }

  double * pr = (double *)alg->alg_data;
  OMP("omp parallel for")
  for(uint64_t v = 0; v < alg->stinger->max_nv; v++) {
    pr[v] = 1 / ((double)alg->stinger->max_nv);
  }

  double * tmp_pr = (double *)xcalloc(alg->stinger->max_nv, sizeof(double));

  double time;
  init_timer();
  
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {
    int64_t type = -1;
    if(type_specified) {
      type = stinger_etype_names_lookup_type(alg->stinger, type_str);
    }
    if(type_specified && type > -1) {
      page_rank_type(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, epsilon, dampingfactor, maxiter, type);
    } else if (!type_specified) {
      page_rank(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, epsilon, dampingfactor, maxiter);
    }
  } stinger_alg_end_init(alg);

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Streaming Phase
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  while(alg->enabled) {
    /* Pre processing */
    if(stinger_alg_begin_pre(alg)) {
      /* nothing to do */
      time = timer();
      stinger_alg_end_pre(alg);
      time = timer() - time;
      LOG_I_A("Pre time : %20.15e", time);
    }

    /* Post processing */
      time = timer();
    if(stinger_alg_begin_post(alg)) {
      int64_t type = -1;
      if(type_specified) {
      	type = stinger_etype_names_lookup_type(alg->stinger, type_str);
      	if(type > -1) {
          page_rank_type(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, epsilon, dampingfactor, maxiter, type);
      	} else {
      	  LOG_W_A("TYPE DOES NOT EXIST %s", type_str);
      	  LOG_W("Existing types:");
          // TODO: Don't go through the loop if LOG_W isn't enabled
      	  for(int64_t t = 0; t < stinger_etype_names_count(alg->stinger); t++) {
      	    LOG_W_A("  > %ld %s", (long) t, stinger_etype_names_lookup_name(alg->stinger, t));
      	  }
      	}
      } else {
        page_rank(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, epsilon, dampingfactor, maxiter);
      }
      stinger_alg_end_post(alg);
    }
    time = timer() - time;
    LOG_I_A("Post time : %20.15e", time);
  }

  LOG_I("Algorithm complete... shutting down");

  free(tmp_pr);
}

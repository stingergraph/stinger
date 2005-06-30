#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"

#define frac(x,y) ((double)(((double)(x))/((double)(y))))

/**
* @brief Perform a single source of the BC calculation per Brandes.
*
* Note that this follows the approach suggested by Green and Bader in which
* parent lists are not maintained, but instead the neighbors are searched
* to rediscover parents.
*
* Note also that found count will not include the source and that increments
* to this array are handled atomically.
*
* Operations to the bc array are NOT ATOMIC
*
* @param S The STINGER graph
* @param nv The maximum vertex ID in the graph plus one.
* @param source The vertex from where this search will start.
* @param bc The array into which the partial BCs will be added.
* @param found_count  An array to track how many times a vertex is found for normalization.
*/
void
single_bc_search(stinger_t * S, int64_t nv, int64_t source, double * bc, int64_t * found_count)
{
  int64_t * paths = (int64_t * )xcalloc(nv * 2, sizeof(int64_t));
  int64_t * q = paths + nv;
  double * partial = (double *)xcalloc(nv, sizeof(double));

  int64_t * d = (int64_t *)xmalloc(nv * sizeof(int64_t));
  for(int64_t i = 0; i < nv; i ++) d[i] = -1;

  int64_t q_front = 1;
  int64_t q_rear = 0;
  q[0] = source;
  d[source] = 0;
  paths[source] = 1;

  while(q_rear != q_front) {
    int64_t v = q[q_rear++];
    int64_t d_next = d[v] + 1;

    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, v) {

      if(d[STINGER_EDGE_DEST] < 0) {
	d[STINGER_EDGE_DEST]     = d_next;
	paths[STINGER_EDGE_DEST] = paths[v];
	q[q_front++]             = STINGER_EDGE_DEST;

	stinger_int64_fetch_add(found_count + STINGER_EDGE_DEST, 1);
      }

      else if(d[STINGER_EDGE_DEST] == d_next) {
	paths[STINGER_EDGE_DEST] += paths[v];
      }

    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  /* don't process source */
  while(q_front > 1) {
    int64_t w = q[--q_front];

    /* don't maintain parents, do search instead */
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, w) {
      if(d[STINGER_EDGE_DEST] == (d[w] - 1)) {
	partial[STINGER_EDGE_DEST] += frac(paths[STINGER_EDGE_DEST],paths[w]) * (1 + partial[w]);
      }
    } STINGER_FORALL_EDGES_OF_VTX_END();
    bc[w] += partial[w];
  }

  free(d);
  free(partial);
  free(paths);
}

void
sample_search(stinger_t * S, int64_t nv, int64_t nsamples, double * bc, int64_t * found_count)
{
  LOG_V_A("  > Beggining with %ld vertices and %ld samples", (long)nv, (long)nsamples);

  int64_t * found = xcalloc(nv, sizeof(int64_t));
  OMP("omp parallel")
  {
    OMP("omp for")
    for(int64_t v = 0; v < nv; v++) {
      found_count[v] = 0;
      bc[v] = 0;
    }

    double * partials = (double *)xcalloc(nv, sizeof(double));

    if(nv * 2 < nsamples) {
      int64_t min = nv < nsamples ? nv : nsamples;

      OMP("omp for")
      for(int64_t s = 0; s < min; s++) {
	single_bc_search(S, nv, s, partials, found_count);
      }
    } else {
      OMP("omp for")
      for(int64_t s = 0; s < nsamples; s++) {
	int64_t v = 0;

	while(1) {
	  v = rand() % nv;
          if(found[v] == 0 && (0 == stinger_int64_fetch_add(found + v, 1))) {
	    break;
	  }
	}

	single_bc_search(S, nv, v, partials, found_count);
      }
    }
    
    OMP("omp critical")
    {
      for(int64_t v = 0; v < nv; v++) {
	bc[v] += partials[v];
      }
    }

    free(partials);
  }

  free(found);
}

int
main(int argc, char *argv[]) 
{
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Options and arg parsing
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  int64_t num_samples = 256;
  double weighting = 0.5;
  uint8_t do_weighted = 1;
  char * alg_name = "betweenness_centrality";

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "w:s:n:x?h"))) {
    switch(opt) {
      case 'w': {
	weighting = atof(optarg);
	if(weighting > 1.0) {
	  weighting = 1.0;
	} else if(weighting < 0.0) {
	  weighting = 0.0;
	}
      } break;
      
      case 's': {
	num_samples = atol(optarg);
	if(num_samples < 1) {
	  num_samples = 1;
	}
      } break;

      case 'n': {
	alg_name = optarg;
      } break;

      case 'x': {
	do_weighted = 0;
      } break;

      default: 
	printf("Unknown option '%c'\n", opt);
      case '?':
      case 'h': {
	printf(
"Approximate Betweenness Centrality\n"
"==================================\n"
"\n"
"Measures an approximate BC on the STINGER graph by randomly sampling n vertices each pass and\n"
"performing one breadth-first traversal from the Brandes algorithm. The samples are then\n"
"aggregated and potentially weighted with the result from\n"
"the last pass\n"
"\n"
"  -s <num>  Set the number of samples (%ld by default)\n"
"  -w <num>  Set the weighintg (0.0 - 1.0) (%lf by default)\n"
"  -x        Disable weighting\n"
"  -n <str>  Set the algorithm name (%s by default)\n"
"\n", num_samples, weighting, alg_name
	);
	return(opt);
      }
    }
  }

  double old_weighting = 1 - weighting;

  LOG_V("Starting approximate betweenness centrality...");

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_registered_alg * alg = 
    stinger_register_alg(
      .name=alg_name,
      .data_per_vertex=sizeof(int64_t) + sizeof(double),
      .data_description="dl bc times_found",
      .host="localhost",
    );

  if(!alg) {
    LOG_E("Registering algorithm failed.  Exiting");
    return -1;
  }

  double * bc = (double *)alg->alg_data;
  int64_t * times_found = (int64_t *)(bc + STINGER_MAX_LVERTICES);
  double * sample_bc = NULL;

  if(do_weighted) {
    sample_bc = xcalloc(sizeof(double), STINGER_MAX_LVERTICES);
  }
  
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {
    sample_search(alg->stinger, stinger_max_active_vertex(alg->stinger) + 1, num_samples, bc, times_found);
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
      int64_t nv = stinger_mapping_nv(alg->stinger)+1;
      if(do_weighted) {
	sample_search(alg->stinger, nv, num_samples, sample_bc, times_found);

	OMP("omp parallel for")
	for(int64_t v = 0; v < nv; v++) {
	  bc[v] = bc[v] * old_weighting + weighting* sample_bc[v];
	}
      } else {
	sample_search(alg->stinger, nv, num_samples, bc, times_found);
      }

      stinger_alg_end_post(alg);
    }
  }

  LOG_I("Algorithm complete... shutting down");

  if(do_weighted) {
    free(sample_bc);
  }
}

#include <stdint.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "stinger_utils/timer.h"


int compare (const void * a, const void * b)
{
    return ( *(int64_t*)a - *(int64_t*)b );
}


/* For the sake of performance, I'm going to assume that the graph is undirected.
 * Therefore I can sort the edge list of the first vertex and re-use it throughout
 * the intersection computations.  If it is not undirected, this will not yield the
 * correct answer (but will compute transitivity coefficients correctly.)
 */

int64_t
//count_intersections (stinger_t * S, int64_t a, int64_t b)
count_intersections (stinger_t * S, int64_t a, int64_t b, int64_t * neighbors, int64_t d)
{
  size_t out = 0;
  
  STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, b) {

    if (STINGER_EDGE_DEST != a) {
      int64_t first = 0;
      int64_t last = d-1;
      int64_t middle = (first + last)/2;

      while (first <= last) {
	if (neighbors[middle] < STINGER_EDGE_DEST) {
	  first = middle + 1;
	} else if (neighbors[middle] == STINGER_EDGE_DEST) {
	  out++;
	  break;
	} else {
	  last = middle - 1;
	}

	middle = (first + last)/2;
      }
      //out += stinger_has_typed_successor (S, 0, STINGER_EDGE_DEST, a);
    }

  } STINGER_FORALL_EDGES_OF_VTX_END();


  return out;
}


int64_t
count_triangles (stinger_t * S, uint64_t v)
{
  int64_t out = 0;

  int64_t deg = stinger_outdegree(S, v);
  int64_t * neighbors = xmalloc(deg * sizeof(int64_t));
  size_t d;
  stinger_gather_typed_successors(S, 0, v, &d, neighbors, deg);
  qsort(neighbors, d, sizeof(int64_t), compare);

  STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, v) {

    if (STINGER_EDGE_DEST != v) {
      //out += count_intersections (S, v, STINGER_EDGE_DEST);
      out += count_intersections (S, v, STINGER_EDGE_DEST, neighbors, d);
    }

  } STINGER_FORALL_EDGES_OF_VTX_END();

  free (neighbors);

  return out;
}


void
count_all_triangles (stinger_t * S, int64_t * ntri)
{
  OMP ("omp for schedule(dynamic,128)")
  for (size_t i = 0; i < S->max_nv; ++i)
    ntri[i] = count_triangles (S, i);
}




int
main(int argc, char *argv[])
{
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Setup and register algorithm with the server
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  char name[1024];
  if(argc > 1) {
    sprintf(name, "clustering_coeff_%s", argv[1]);
  }

  stinger_registered_alg * alg = 
    stinger_register_alg(
      .name=argc > 1 ? name : "clustering_coeff",
      .data_per_vertex=(sizeof(double)+sizeof(int64_t)),
      .data_description="dl coeff ntriangles",
      .host="localhost",
    );

  if(!alg) {
    LOG_E("Registering algorithm failed.  Exiting");
    return -1;
  }

  double * local_cc = (double *)alg->alg_data;
  int64_t * ntri = (int64_t *)(((double *)alg->alg_data) + alg->stinger->max_nv);

  int64_t * affected = xcalloc (alg->stinger->max_nv, sizeof (int64_t *));

  init_timer();
  double time;
  
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Initial static computation
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  stinger_alg_begin_init(alg); {
    LOG_I("Clustering coefficients init starting");
    tic();
    count_all_triangles (alg->stinger, ntri);

    OMP("omp parallel for")
    for(uint64_t v = 0; v < alg->stinger->max_nv; v++) {
      int64_t deg = stinger_outdegree_get(alg->stinger, v);
      int64_t d = deg * (deg-1);
      local_cc[v] = (d ? ntri[v] / (double) d : 0.0);
    }
    time = toc();
    LOG_I("Clustering coefficients init end");
    LOG_I_A("Time: %f sec", time);
  } stinger_alg_end_init(alg);

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
   * Streaming Phase
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  while(alg->enabled) {
    /* Pre processing */
    if(stinger_alg_begin_pre(alg)) {

      tic();

      OMP("omp parallel for")
      for (uint64_t v = 0; v < alg->stinger->max_nv; v++) {
	affected[v] = 0;
      }

      /* each vertex incident on an insertion is affected */
      OMP("omp parallel for")
      for (uint64_t i = 0; i < alg->num_insertions; i++) {
	int64_t src = alg->insertions[i].source;
	int64_t dst = alg->insertions[i].destination;
	stinger_int64_fetch_add (&affected[src], 1);
	stinger_int64_fetch_add (&affected[dst], 1);

	/* and all neighbors */
	STINGER_FORALL_EDGES_OF_VTX_BEGIN(alg->stinger, src) {
	  stinger_int64_fetch_add (&affected[STINGER_EDGE_DEST], 1);
	} STINGER_FORALL_EDGES_OF_VTX_END();
	
	STINGER_FORALL_EDGES_OF_VTX_BEGIN(alg->stinger, dst) {
	  stinger_int64_fetch_add (&affected[STINGER_EDGE_DEST], 1);
	} STINGER_FORALL_EDGES_OF_VTX_END();
      }
     
      /* each vertex incident on a deletion is affected */
      OMP("omp parallel for")
      for (uint64_t i = 0; i < alg->num_deletions; i++) {
	int64_t src = alg->deletions[i].source;
	int64_t dst = alg->deletions[i].destination;
	stinger_int64_fetch_add (&affected[src], 1);
	stinger_int64_fetch_add (&affected[dst], 1);
	
	/* and all neighbors */
	STINGER_FORALL_EDGES_OF_VTX_BEGIN(alg->stinger, src) {
	  stinger_int64_fetch_add (&affected[STINGER_EDGE_DEST], 1);
	} STINGER_FORALL_EDGES_OF_VTX_END();
	
	STINGER_FORALL_EDGES_OF_VTX_BEGIN(alg->stinger, dst) {
	  stinger_int64_fetch_add (&affected[STINGER_EDGE_DEST], 1);
	} STINGER_FORALL_EDGES_OF_VTX_END();
      }

      time = toc();
      LOG_I_A("Time: %f", time);

      /* nothing to do */
      stinger_alg_end_pre(alg);
    }

    /* Post processing */
    if(stinger_alg_begin_post(alg)) {

      tic();

      OMP("omp parallel for")
      for (uint64_t v = 0; v < alg->stinger->max_nv; v++) {
	if (affected[v]) {
	  ntri[v] = count_triangles (alg->stinger, v);
	  
	  int64_t deg = stinger_outdegree_get(alg->stinger, v);
	  int64_t d = deg * (deg-1);
	  local_cc[v] = (d ? ntri[v] / (double) d : 0.0);
	}
      }

      time = toc();
      LOG_I_A("Time: %f", time);

      /* done */
      stinger_alg_end_post(alg);
    }
  }

  LOG_I("Algorithm complete... shutting down");

}

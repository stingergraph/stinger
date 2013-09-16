#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <queue>
#include <utility>

#include "int_ht_seq/int_ht_seq.h"
#include "int_hm_seq/int_hm_seq.h"

#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/stinger.h"

typedef struct {
  int64_t type;
  int64_t src;
  int64_t dest;
} local_edge_t;

template<bool postpone_deletes, bool filter_from_similarity>
int64_t
depold_community_step(stinger_t * S, int64_t nv, int64_t deg_thresh, double simil_thresh)
{
  printf("Called\n");

  double max2hop = deg_thresh * deg_thresh;

  int64_t removed_edges = 0;

  OMP("omp parallel reduction(+:removed_edges)")
  {
    std::queue<local_edge_t> deletions;

    OMP("omp for")
    for(uint64_t n = 0; n < nv; n++) {
      if(stinger_outdegree(S, n) < deg_thresh) {
	int_ht_seq_t neighbors;
	int_ht_seq_init(&neighbors, deg_thresh);

	STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, n) {
	  if((!filter_from_similarity) || (stinger_outdegree(S, STINGER_EDGE_DEST) < deg_thresh))
	    int_ht_seq_insert(&neighbors, STINGER_EDGE_DEST);
	} STINGER_FORALL_EDGES_OF_VTX_END();

	STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, n) {
	  /* comparing IDs done to enforce computing similarity only once per edge 
	   * assuming that edges are undirected */
	  if(stinger_outdegree(S, STINGER_EDGE_DEST) < deg_thresh) {
	    int_ht_seq_t two_hoppers;
	    int_ht_seq_init(&two_hoppers, max2hop);

	    int64_t l = STINGER_EDGE_DEST;
	    int64_t common_neighbor_edges = 0;

	    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, l) {
	      printf("TESTING %ld - %ld - %ld\n", (long)n, (long)l, (long)STINGER_EDGE_DEST);
	      if(int_ht_seq_exists(&neighbors, STINGER_EDGE_DEST)) {
		printf("TRIANGLE %ld - %ld - %ld\n", (long)n, (long)l, (long)STINGER_EDGE_DEST);
		int_ht_seq_insert(&two_hoppers, STINGER_EDGE_DEST);
		int64_t c = STINGER_EDGE_DEST;

		STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, c) {
		  if(int_ht_seq_exists(&two_hoppers, STINGER_EDGE_DEST)) {
		    common_neighbor_edges++;
		  }
		} STINGER_FORALL_EDGES_OF_VTX_END();
	      }
	    } STINGER_FORALL_EDGES_OF_VTX_END();

	    double similarity = 1.0 + ((double)two_hoppers.elements) + ((double)common_neighbor_edges);
	    similarity = ((double)similarity)/((double)(stinger_outdegree(S,n) + stinger_outdegree(S, l)));

	    printf("%ld to %ld is %lf with denom %ld\n", (long)n, (long)STINGER_EDGE_DEST, similarity, (long)(stinger_outdegree(S,n) + stinger_outdegree(S,l)));

	    if(similarity < simil_thresh) {
	      if(!postpone_deletes) {
		stinger_remove_edge_pair(S, STINGER_EDGE_TYPE, n, l);
	      } else {
		local_edge_t edge = {STINGER_EDGE_TYPE, n, l};
		deletions.push(edge);
	      }
	      removed_edges++;
	    }

	    int_ht_seq_free_internal(&two_hoppers);
	  }
	} STINGER_FORALL_EDGES_OF_VTX_END();

	int_ht_seq_free_internal(&neighbors);
      }
    }

    while(postpone_deletes && !deletions.empty()) {
      local_edge_t & edge = deletions.front();
      stinger_remove_edge_pair(S, edge.type, edge.src, edge.dest);
      deletions.pop();
    }

  } /* end parallel section */

  printf("returning %ld\n", (long)removed_edges);
  return removed_edges;
}

static void
depold_cc(stinger_t * S, int64_t nv, int64_t deg_thresh, int64_t * labels)
{
  /* Initialize each vertex with its own component label in parallel */
  OMP ("omp parallel for")
    for (uint64_t i = 0; i < nv; i++) {
      labels[i] = i;
    }

  /* Iterate until no changes occur */
  while (1) {
    int changed = 0;

    /* For all edges in the STINGER graph of type 0 in parallel, attempt to assign
       lesser component IDs to neighbors with greater component IDs */
    for(int64_t t = 0; t < STINGER_NUMETYPES; t++) {
      STINGER_PARALLEL_FORALL_EDGES_BEGIN (S, t) {
	if ((labels[STINGER_EDGE_DEST] < labels[STINGER_EDGE_SOURCE]) && 
	  (stinger_outdegree(S, STINGER_EDGE_SOURCE) < deg_thresh) &&
	  (stinger_outdegree(S, STINGER_EDGE_DEST) < deg_thresh)) {
	  labels[STINGER_EDGE_SOURCE] = labels[STINGER_EDGE_DEST];
	  changed++;
	}
      }
      STINGER_PARALLEL_FORALL_EDGES_END ();
    }

    /* if nothing changed */
    if (!changed)
      break;

    /* Tree climbing with OpenMP parallel for */
    OMP ("omp parallel for")
      MTA ("mta assert nodep")
      for (uint64_t i = 0; i < nv; i++) {
        while (labels[i] != labels[labels[i]])
          labels[i] = labels[labels[i]];
      }
  }
}

int64_t
depold_post(stinger_t * S, int64_t nv, int64_t deg_thresh, int64_t * labels)
{
  int64_t * total_deg = (int64_t *)xcalloc(nv, sizeof(int64_t));
  int64_t * members = (int64_t *)xcalloc(nv, sizeof(int64_t));

  OMP("omp parallel for")
  for(int64_t v = 0; v < nv; v++) {
    if(stinger_outdegree(S, v) < deg_thresh) {
      stinger_int64_fetch_add(members + labels[v], 1);
    }
  }

  for(int64_t t = 0; t < STINGER_NUMETYPES; t++) {
    STINGER_PARALLEL_FORALL_EDGES_BEGIN (S, t) {
      if (labels[STINGER_EDGE_DEST] == labels[STINGER_EDGE_SOURCE]) {
	stinger_int64_fetch_add(total_deg + labels[STINGER_EDGE_DEST], 1);
      }
    }
    STINGER_PARALLEL_FORALL_EDGES_END ();
  }

  OMP("omp parallel for")
  for(int64_t v = 0; v < nv; v++) {
    if(stinger_outdegree(S, v) >= deg_thresh) {
      int_hm_seq_t * groups = int_hm_seq_new(deg_thresh * deg_thresh);

      STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, v) {
	if(stinger_outdegree(S, STINGER_EDGE_DEST) < deg_thresh) {
	  int64_t * group = int_hm_seq_get_location(groups, labels[STINGER_EDGE_DEST]);
	  if(*group == INT_HT_SEQ_EMPTY) {
	    *group = 1;
	  } else {
	    (*group)++;
	  }
	}
      } STINGER_FORALL_EDGES_OF_VTX_END();

      int64_t max = 0;
      int64_t max_label = 0;

      for(int64_t i = 0; i < groups->size; i++) {
	if(groups->vals[i] != INT_HT_SEQ_EMPTY) {
	  printf("COMPARING %lf %lf %lf\n", ((double)groups->vals[i]), ((double)total_deg[groups->keys[i]]), ((double)members[groups->keys[i]]));
	  if(((double)groups->vals[i]) > (((double)total_deg[groups->keys[i]]) / ((double)members[groups->keys[i]]))) {
	    if(groups->vals[i] > max) {
	      max = groups->vals[i];
	      max_label = groups->keys[i];
	    }
	  }
	}
      }

      if(max > 0)
	labels[v] = max_label;

      int_hm_seq_free(groups);
    }
  }

  free(total_deg);
  free(members);
}

int64_t
depold_community(stinger_t * S, int64_t nv, int64_t deg_thresh, double simil_thresh, double edges_thresh, int64_t * labels)
{
  deg_thresh++; /* their threshold is inclusive */

  int64_t iterations = 0;
  while(edges_thresh < depold_community_step<false,true>(S, nv, deg_thresh, simil_thresh)) {
    /* run until convergence */
    iterations++;
  }

  depold_cc(S, nv, deg_thresh, labels);

  depold_post(S, nv, deg_thresh, labels);

  return iterations;
}

int
main(int argc, char *argv[])
{
  stinger_t * S = stinger_new();

  stinger_insert_edge_pair(S, 0, 1, 3, 1, 1);
  stinger_insert_edge_pair(S, 0, 2, 3, 1, 1);
  stinger_insert_edge_pair(S, 0, 6, 3, 1, 1);
  stinger_insert_edge_pair(S, 0, 7, 3, 1, 1);
  stinger_insert_edge_pair(S, 0, 7, 6, 1, 1);
  stinger_insert_edge_pair(S, 0, 7, 12, 1, 1);
  stinger_insert_edge_pair(S, 0, 7, 17, 1, 1);
  stinger_insert_edge_pair(S, 0, 7, 16, 1, 1);
  stinger_insert_edge_pair(S, 0, 12, 16, 1, 1);
  stinger_insert_edge_pair(S, 0, 17, 16, 1, 1);
  stinger_insert_edge_pair(S, 0, 17, 12, 1, 1);
  stinger_insert_edge_pair(S, 0, 16, 19, 1, 1);
  stinger_insert_edge_pair(S, 0, 16, 18, 1, 1);
  stinger_insert_edge_pair(S, 0, 16, 15, 1, 1);
  stinger_insert_edge_pair(S, 0, 19, 18, 1, 1);
  stinger_insert_edge_pair(S, 0, 21, 18, 1, 1);
  stinger_insert_edge_pair(S, 0, 15, 18, 1, 1);
  stinger_insert_edge_pair(S, 0, 5, 6, 1, 1);
  stinger_insert_edge_pair(S, 0, 5, 15, 1, 1);
  stinger_insert_edge_pair(S, 0, 5, 11, 1, 1);
  stinger_insert_edge_pair(S, 0, 5, 10, 1, 1);
  stinger_insert_edge_pair(S, 0, 11, 10, 1, 1);
  stinger_insert_edge_pair(S, 0, 15, 10, 1, 1);
  stinger_insert_edge_pair(S, 0, 15, 11, 1, 1);

  int64_t nv = 22;
  int64_t * labels = (int64_t *)xcalloc(nv, sizeof(int64_t));
  depold_community(S, nv, 5, 0.2, 1, labels);

  for(int64_t v = 0; v < nv; v++) {
    printf("label[%ld] = %ld\n", (long)v, (long)labels[v]);
  }

  free(labels);
}

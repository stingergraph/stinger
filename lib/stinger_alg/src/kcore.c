#include <stdio.h>
#include "kcore.h"

void
kcore_find(stinger_t *S, int64_t * labels, int64_t * counts, int64_t nv, int64_t * k_out) {
  int64_t k = 0;

  for(int64_t v = 0; v < nv; v++) {
    if(stinger_outdegree_get(S,v)) {
      labels[v] = 1;
    } else {
      labels[v] = 0;
    }
  }

  int changed = 1;
  while(changed) {
    changed = 0;
    k++;

    OMP("omp parallel for")
      for(int64_t v = 0; v < nv; v++) {
      	if(labels[v] == k) {
      	  int64_t count = 0;
      	  STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, v) {
      	    count += (labels[STINGER_EDGE_DEST] >= k);
      	  } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
      	  if(count > k) {
      	    counts[v] = count;
          }
      	}
      }

    OMP("omp parallel for")
      for(int64_t v = 0; v < nv; v++) {
      	if(labels[v] == k) {
      	  int64_t count = 0;
      	  STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, v) {
      	    count += (labels[STINGER_EDGE_DEST] >= k &&
      		counts[STINGER_EDGE_DEST] > k);
      	  } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
      	  if(count > k) {
      	    labels[v] = k+1;
      	    changed = 1;
      	  }
      	}
      }
  }

  *k_out = k;
}

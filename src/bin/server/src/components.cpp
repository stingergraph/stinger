#include <cstdio>
#include <algorithm>
#include <limits>
#include <unistd.h>

extern "C" {
  #include "stinger_core/stinger.h"
  #include "stinger_core/stinger_atomics.h"
  #include "stinger_core/xmalloc.h"
}

#include "server.h"


void
components_init(struct stinger * S, int64_t nv, int64_t * component_map) {
  /* Initialize each vertex with its own component label in parallel */
  OMP ("omp parallel for")
    for (uint64_t i = 0; i < nv; i++) {
      component_map[i] = i;
    }
  
  /* S is empty... components_batch(S, nv, component_map); */
  n_components = nv;
}

void
components_batch(struct stinger * S, int64_t nv, int64_t * component_map) {
  int64_t nc;
  /* Iterate until no changes occur */
  while (1) {
    int changed = 0;

    /* For all edges in the STINGER graph of type 0 in parallel, attempt to assign
       lesser component IDs to neighbors with greater component IDs */
    for(uint64_t t = 0; t < STINGER_NUMETYPES; t++) {
      STINGER_PARALLEL_FORALL_EDGES_BEGIN (S, t) {
	if (component_map[STINGER_EDGE_DEST] <
	    component_map[STINGER_EDGE_SOURCE]) {
	  component_map[STINGER_EDGE_SOURCE] = component_map[STINGER_EDGE_DEST];
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
	while (component_map[i] != component_map[component_map[i]])
          component_map[i] = component_map[component_map[i]];
      }
  }
  nc = 0;
  OMP ("omp parallel for reduction(+: nc)")
    for (uint64_t i = 0; i < nv; i++)
      if (component_map[i] == component_map[component_map[i]]) ++nc;
  n_components = nc;

  n_nonsingleton_components = 0;
  max_compsize = 0;
  int64_t nnsc = 0, mxcsz = 0;
  if (nc) {
    int64_t nvlist = 0;
    int64_t * vlist = comp_vlist;
    int64_t * mark = comp_mark;
    OMP("omp parallel") {
      OMP("omp for")
	for (int64_t k = 0; k < nv; ++k) {
	  const int64_t c = component_map[k];
	  assert (c < n_components);
	  assert (c >= 0);
	  int64_t subcnt = stinger_int64_fetch_add (&mark[c], 1);
	  if (-1 == subcnt) { /* First to claim. */
	    int64_t where = stinger_int64_fetch_add (&nvlist, 1);
	    assert (where < n_components);
	    assert (where >= 0);
	    vlist[where] = c;
	  }
	}

      int64_t cmaxsz = -1;
      OMP("omp for reduction(+: nnsc)")
	for (int64_t k = 0; k < nvlist; ++k) {
	  const int64_t c = vlist[k];
	  assert (c < n_components);
	  assert (c >= 0);
	  const int64_t csz = mark[c]+1;
	  if (csz > 1) ++nnsc;
	  if (csz > cmaxsz) cmaxsz = csz;
	  mark[c] = -1; /* Reset to -1. */
	}
      OMP("omp critical")
	if (cmaxsz > mxcsz) mxcsz = cmaxsz;

#if !defined(NDEBUG)
      OMP("omp for") for (int64_t k = 0; k < n_components; ++k) assert (mark[k] == -1);
#endif
    }
    /* XXX: not right, but cannot find where these are appearing as
       zero in the stats display... */
    if (nnsc)
      n_nonsingleton_components = nnsc;
    if (mxcsz)
      max_compsize = mxcsz;
  }
}


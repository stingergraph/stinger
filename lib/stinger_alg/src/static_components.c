#include <stdio.h>
#include "static_components.h"

/*
 * Perform a shiloach vishkin connected components calculation in parallel on a stinger graph
 */
int64_t
parallel_shiloach_vishkin_components (struct stinger * S, int64_t nv,
                                      int64_t * component_map)
{
  /* Initialize each vertex with its own component label in parallel */
  OMP ("omp parallel for")
    for (uint64_t i = 0; i < nv; i++) {
      component_map[i] = i;
    }

  /* Iterate until no changes occur */
  while (1) {
    int changed = 0;

    /* For all edges in the STINGER graph of type 0 in parallel, attempt to assign
       lesser component IDs to neighbors with greater component IDs */
    for(int64_t t = 0; t < S->max_netypes; t++) {
      STINGER_PARALLEL_FORALL_EDGES_BEGIN (S, t) {
        if (component_map[STINGER_EDGE_DEST] < component_map[STINGER_EDGE_SOURCE]) {
          component_map[STINGER_EDGE_SOURCE] = component_map[STINGER_EDGE_DEST];
          changed++;
        }
      }
      STINGER_PARALLEL_FORALL_EDGES_END ();
    }

    /* if nothing changed */
    if (!changed) {
      break;
    }

    /* Tree climbing with OpenMP parallel for */
    OMP ("omp parallel for")
      MTA ("mta assert nodep")
      for (uint64_t i = 0; i < nv; i++) {
        while (component_map[i] != component_map[component_map[i]]) {
          component_map[i] = component_map[component_map[i]];
        }
      }
  }

  return 0;
}

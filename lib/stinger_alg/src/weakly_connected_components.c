#include <stdio.h>
#include "weakly_connected_components.h"


/*
 * Perform a shiloach vishkin connected components calculation in parallel on a stinger graph
 */
int64_t
parallel_shiloach_vishkin_components_of_type (struct stinger * S,  int64_t * component_map, int64_t type)
{
  int64_t nv = S->max_nv;

  if (type < 0) {
    return -1;
  }

  if (type >= S->max_netypes) {
    return -1;
  }

  /* Initialize each vertex with its own component label in parallel */
  OMP ("omp parallel for")
  for (uint64_t i = 0; i < nv; i++) {
    component_map[i] = i;
  }

  /* Iterate until no changes occur */
  while (1) {
    int changed = 0;

    /* For all edges in the STINGER graph of type in parallel, attempt to assign
       lesser component IDs to neighbors with greater component IDs */
    STINGER_PARALLEL_FORALL_EDGES_BEGIN (S, type) {
      int64_t c_src  = component_map[STINGER_EDGE_SOURCE];
      int64_t c_dest = component_map[STINGER_EDGE_DEST];
      /* handles both edge directions */
      if (c_dest < c_src) {
        component_map[STINGER_EDGE_SOURCE] = c_dest;
        changed++;
      }
      if (c_src < c_dest) {
        component_map[STINGER_EDGE_DEST] = c_src;
        changed++;
      }
    } STINGER_PARALLEL_FORALL_EDGES_END ();

    /* if nothing changed */
    if (!changed) {
      break;
    }

    /* Tree climbing with OpenMP parallel for */
    OMP ("omp parallel for")
    for (uint64_t i = 0; i < nv; i++) {
      while (component_map[i] != component_map[component_map[i]]) {
	       component_map[i] = component_map[component_map[i]];
      }
    }
  }

  return 0;
}


/*
 * Compute vertex counts per component
 */
int64_t
compute_component_sizes (struct stinger * S,  int64_t * component_map, int64_t * component_size)
{
  int64_t nv = S->max_nv;

  OMP ("omp parallel for")
  for (uint64_t i = 0; i < nv; i++) {
    component_size[i] = 0;
  }

  OMP ("omp parallel for")
  for (uint64_t i = 0; i < nv; i++) {
    int64_t c_num = component_map[i];
    stinger_int64_fetch_add(&component_size[c_num], 1);
  }

  return 0;
}

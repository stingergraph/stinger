#include "community_on_demand.h"
#include "modularity.h"
#include "stinger_core/stinger_error.h"

/*
 *  On-demand community detection
 *
 */
int64_t community_on_demand(const stinger_t * S, int64_t ** vertices, int64_t ** partitions) {
  if (*vertices != NULL || *partitions != NULL) {
    LOG_E("Community on demand output arrays should not be allocated before call.  Possible memory leak.");
  }

  int64_t * output_vertices = NULL;
  int64_t * output_partitions = NULL;

  /* Initialize to NULL in case of error */
  *vertices = output_vertices;
  *partitions = output_partitions;

  /* Allocate output array */
  int64_t nv = stinger_max_active_vertex(S);
  output_vertices = (int64_t *) xmalloc (nv * sizeof(int64_t));
  output_partitions = (int64_t *) xmalloc (nv * sizeof(int64_t));

  /* Initialize vertex numbers */
  int64_t v;
  for (v = 0; v < nv; v += 1) {
      output_vertices[v] = v;
  }

  /* Note wiring of max iterations to 5 */
  community_detection(S, nv, output_partitions, (int64_t) 5);

  /* Set output arrays */
  *vertices = output_vertices;
  *partitions = output_partitions;

  return nv;
}

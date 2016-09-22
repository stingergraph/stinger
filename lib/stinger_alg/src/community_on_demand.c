#include "community_on_demand.h"
#include "stinger_core/stinger_error.h"

/*
 *  Community-on-demand stub
 *
 */
int64_t community_on_demand(const stinger_t * S, int64_t ** vertices, int64_t ** partitions) {
  if (*vertices != NULL || *partitions != NULL) {
    LOG_E("Community on demand output arrays should not be allocated before call.  Possible memory leak.");
  }

  int64_t * output_vertices = NULL;
  int64_t * output_partitions = NULL;

  int64_t fake_vtx;

  /* Initialize to NULL in case of error */
  *vertices = output_vertices;
  *partitions = output_partitions;

  /* Allocate output array */
  output_vertices = (int64_t *) xmalloc (10 * sizeof(int64_t));
  output_partitions = (int64_t *) xmalloc (10 * sizeof(int64_t));

  /* Set fake values */
  for (fake_vtx = 0; fake_vtx < 10; fake_vtx += 1) {
      output_vertices[fake_vtx] = fake_vtx;
      output_partitions[fake_vtx] = fake_vtx;
  }
  /* Set output arrays */
  *vertices = output_vertices;
  *partitions = output_partitions;

  return 10;
}

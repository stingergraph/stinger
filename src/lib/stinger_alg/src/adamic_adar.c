#include "adamic_adar.h"
#include "stinger_core/stinger_error.h"

static int
compare (const void * a, const void * b)
{
    return ( *(int*)a - *(int*)b );
}

int64_t adamic_adar(const stinger_t * S, int64_t source, int64_t etype, int64_t ** candidates, double ** scores) {
  if (*candidates != NULL || *scores != NULL) {
    LOG_E("Adamic Adar output arrays should not be allocated before call.  Possible memory leak.");
  }

  int64_t * output_vertices = NULL;
  double * output_scores = NULL;

  int64_t source_deg = stinger_outdegree (S, source);

  /* Initialize to NULL in case of error */
  *candidates = output_vertices;
  *scores = output_scores;

  /* vertex has no edges -- this is easy */
  if (source_deg == 0) {
    return 0;
  }

  int64_t * source_adj = (int64_t *) xmalloc (source_deg * sizeof(int64_t));

  /* extract and sort the neighborhood of SOURCE */
  size_t res;
  if (etype == -1) {
    stinger_gather_successors (S, source, &res, source_adj, NULL, NULL, NULL, NULL, source_deg);
  } else {
    stinger_gather_typed_successors (S, etype, source, &res, source_adj, source_deg);
    source_deg = res;
  }
  qsort (source_adj, source_deg, sizeof(int64_t), compare);

  /* create a marks array to unique-ify the vertex list */
  uint64_t max_nv = S->max_nv;
  int64_t * marks = (int64_t *) xmalloc (max_nv * sizeof(int64_t));

  for (int64_t i = 0; i < max_nv; i++) {
    marks[i] = 0;
  }
  marks[source] = 1;

  /* calculate the maximum number of vertices in the two-hop neighborhood */
  int64_t two_hop_size = 0;
  for (int64_t i = 0; i < source_deg; i++) {
    int64_t v = source_adj[i];
    int64_t v_deg = stinger_outdegree(S, v);
    marks[v] = 1;
    two_hop_size += v_deg;
  }


  /* put everyone in the two hop neighborhood in a list (only once) */
  int64_t head = 0;
  int64_t * two_hop_neighborhood = (int64_t *) xmalloc (two_hop_size * sizeof(int64_t));

  for (int64_t i = 0; i < source_deg; i++) {
    int64_t v = source_adj[i];
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S,v) {
      if (stinger_int64_fetch_add(&marks[STINGER_EDGE_DEST], 1) == 0) {
      	int64_t loc = stinger_int64_fetch_add(&head, 1);
      	two_hop_neighborhood[loc] = STINGER_EDGE_DEST;
      }
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  /* sanity check */
  if (head > two_hop_size) {
    LOG_W ("over ran the buffer");
  }
  two_hop_size = head;  // limit the computation later

  /* Allocate output array */
  output_vertices = (int64_t *) xmalloc (two_hop_size * sizeof(int64_t));
  output_scores = (int64_t *) xmalloc (two_hop_size * sizeof(double));

  /* calculate the Adamic-Adar score for each vertex in level 2 */
  for (int64_t k = 0; k < two_hop_size; k++) {
    double adamic_adar_score = 0.0;
    int64_t vtx = two_hop_neighborhood[k];
    int64_t v_deg = stinger_outdegree(S, vtx);

    int64_t * target_adj = (int64_t *) xmalloc (v_deg * sizeof(int64_t));
    size_t res;
    if (etype == -1) {
      stinger_gather_successors (S, vtx, &res, target_adj, NULL, NULL, NULL, NULL, v_deg);
    } else {
      stinger_gather_typed_successors (S, etype, vtx, &res, target_adj, v_deg);
      v_deg = res;
    }
    qsort (target_adj, v_deg, sizeof(int64_t), compare);

    int64_t i = 0, j = 0;
    while (i < source_deg && j < v_deg)
    {
      if (source_adj[i] < target_adj[j]) {
        i++;
      }
      else if (target_adj[j] < source_adj[i]) {
        j++;
      }
      else /* if source_adj[i] == target_adj[j] */
      {
      	int64_t neighbor = source_adj[i];
      	int64_t my_deg = stinger_outdegree(S, neighbor);
        double score = 0.0;
        if (my_deg > 1) {
          score = 1.0 / log ((double) my_deg);
        }
      	adamic_adar_score += score;
      	i++;
      	j++;
      }
    }

    free (target_adj);

    output_vertices[k] = vtx;
    output_scores[k] = adamic_adar_score;
  }

  /* Set output arrays */
  *candidates = output_vertices;
  *scores = output_scores;

  /* done with computation, free memory */
  free (source_adj);
  free (marks);
  free (two_hop_neighborhood);
  return two_hop_size;
}
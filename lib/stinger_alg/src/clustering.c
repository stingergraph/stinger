#include "clustering.h"

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

  STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, b) {

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

  } STINGER_FORALL_OUT_EDGES_OF_VTX_END();


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

  STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, v) {

    if (STINGER_EDGE_DEST != v) {
      //out += count_intersections (S, v, STINGER_EDGE_DEST);
      out += count_intersections (S, v, STINGER_EDGE_DEST, neighbors, d);
    }

  } STINGER_FORALL_OUT_EDGES_OF_VTX_END();

  free (neighbors);

  return out;
}


void
count_all_triangles (stinger_t * S, int64_t * ntri)
{
  OMP ("omp for schedule(dynamic,128)")
  for (size_t i = 0; i < S->max_nv; ++i){
    ntri[i] = count_triangles (S, i);
  }
}

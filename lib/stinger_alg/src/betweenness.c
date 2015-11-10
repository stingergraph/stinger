#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "betweenness.h"

#define PHASE_END -1

void 
single_bc_search(stinger_t * S, int64_t nv, int64_t source, double * bc, int64_t * found_count, uint8_t * vertex_set)
{
  #define USING_SET (vertex_set != NULL)

  int64_t * paths = (int64_t * )xcalloc(nv * 3, sizeof(int64_t));
  int64_t * bfs_stack = paths + nv;
  double * partial = (double *)xcalloc(nv, sizeof(double));

  int64_t * d = (int64_t *)xmalloc(nv * sizeof(int64_t));

  if (paths == NULL || partial == NULL || d == NULL) {
    LOG_E("Could not allocate memory for BC search");
    return;
  }

  for(int64_t i = 0; i < nv; i ++) d[i] = -1;

  int64_t stack_top = 2;
  bfs_stack[0] = source;
  bfs_stack[1] = PHASE_END;
  d[source] = 0;
  paths[source] = 1;
  int64_t count = 1;

  int64_t index = -1;

  //LOG_D("Directed");

  while (count > 0) {
    count = 0;
    index++;

    while (bfs_stack[index] != PHASE_END) {
      int64_t v = bfs_stack[index];
      int64_t d_next = d[v] + 1;

      //LOG_D_A("(%ld) TOS=%ld",source,v);

      STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, v) {
        if (!USING_SET || vertex_set[STINGER_EDGE_DEST]) {
          //LOG_D_A("(%ld) Begin - d[%ld]=%ld paths[%ld]=%ld",source,STINGER_EDGE_DEST,d[STINGER_EDGE_DEST],STINGER_EDGE_DEST,paths[STINGER_EDGE_DEST]);
          if(d[STINGER_EDGE_DEST] < 0) {
            d[STINGER_EDGE_DEST]   = d_next;
            paths[STINGER_EDGE_DEST] = paths[v];
            count = count + 1;
            bfs_stack[stack_top++]       = STINGER_EDGE_DEST;

            stinger_int64_fetch_add(found_count + STINGER_EDGE_DEST, 1);
          }
          else if(d[STINGER_EDGE_DEST] == d_next) {
            paths[STINGER_EDGE_DEST] += paths[v];
          }
          //LOG_D_A("(%ld) End - d[%ld]=%ld paths[%ld]=%ld",source,STINGER_EDGE_DEST,d[STINGER_EDGE_DEST],STINGER_EDGE_DEST,paths[STINGER_EDGE_DEST]);
        }
      } STINGER_FORALL_EDGES_OF_VTX_END();

      index++;
    }

    bfs_stack[stack_top++] = PHASE_END;
  }
  stack_top--;

  while (stack_top > 0) {
    while (bfs_stack[stack_top] != PHASE_END) {
      int64_t w = bfs_stack[stack_top];
      double dsw = 0;
      int64_t sw = paths[w];
      STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, w) {
        if (!USING_SET || vertex_set[STINGER_EDGE_DEST]) {
          if(d[w] == (d[STINGER_EDGE_DEST] - 1)) {
            dsw += frac(sw,paths[STINGER_EDGE_DEST]) * (1.0 + partial[STINGER_EDGE_DEST]);
          }
        }
      } STINGER_FORALL_EDGES_OF_VTX_END();
      partial[w] = dsw;
      bc[w] += dsw;
      stack_top--;
      //LOG_D_A("(%ld) Sum Partials for %ld -- partial[%ld]=%lf bc[%ld]=%lf",source,w,w,partial[w],w,bc[w]);
    }
    stack_top--;
  }

  xfree(d);
  xfree(partial);
  xfree(paths);
  #undef USING_SET
}

void
sample_search(stinger_t * S, int64_t nv, int64_t nsamples, double * bc, int64_t * found_count)
{
  sample_search_subgraph(S, nv, NULL, nsamples, bc, found_count);
}

void
sample_search_subgraph(stinger_t * S, int64_t nv, uint8_t * vertex_set, int64_t nsamples, double * bc, int64_t * found_count)
{
  LOG_V_A("  > Beginning with %ld vertices and %ld samples\n", (long)nv, (long)nsamples);

  #define USING_SET (vertex_set != NULL)

  int64_t set_nv = nv;

  if (USING_SET) {
    set_nv = 0;
    OMP("omp parallel for reduction(+:set_nv)")
    for(int64_t v = 0; v < nv; v++) {
      if (vertex_set[v]) {
        set_nv++;
      }
    }
  }

  int64_t * found = xcalloc(nv, sizeof(int64_t));
  OMP("omp parallel")
  {
    OMP("omp for")
    for(int64_t v = 0; v < nv; v++) {
      found_count[v] = 0;
      bc[v] = 0;
    }

    double * partials = (double *)xcalloc(nv, sizeof(double));

    if(set_nv < nsamples) {
      int64_t min = nv;

      OMP("omp for")
      for(int64_t s = 0; s < min; s++) {
        if (!USING_SET || vertex_set[s]) {
          single_bc_search(S, nv, s, partials, found_count, vertex_set);
        }
      }
    } else {
      OMP("omp for")
      for(int64_t s = 0; s < nsamples; s++) {
        int64_t v = 0;

        while(1) {
          v = rand() % nv;
          if (USING_SET && !vertex_set[v]) {
            continue;
          }
          if(found[v] == 0 && (0 == stinger_int64_fetch_add(found + v, 1))) {
            break;
          }
        }

        single_bc_search(S, nv, v, partials, found_count, vertex_set);
      }
    }

    OMP("omp critical")
    {
      for(int64_t v = 0; v < nv; v++) {
        bc[v] += partials[v];
      }
    }

    free(partials);
  }

  free(found);

  #undef USING_SET
}

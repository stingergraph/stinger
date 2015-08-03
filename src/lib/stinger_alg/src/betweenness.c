#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
#include "betweenness.h"

/**
* @brief Perform a single source of the BC calculation per Brandes.
*
* Note that this follows the approach suggested by Green and Bader in which
* parent lists are not maintained, but instead the neighbors are searched
* to rediscover parents.
*
* Note also that found count will not include the source and that increments
* to this array are handled atomically.
*
* Operations to the bc array are NOT ATOMIC
*
* @param S The STINGER graph
* @param nv The maximum vertex ID in the graph plus one.
* @param source The vertex from where this search will start.
* @param bc The array into which the partial BCs will be added.
* @param found_count  An array to track how many times a vertex is found for normalization.
*/
void
single_bc_search(stinger_t * S, int64_t nv, int64_t source, double * bc, int64_t * found_count)
{
    int64_t * paths = (int64_t * )xcalloc(nv * 2, sizeof(int64_t));
    int64_t * q = paths + nv;
    double * partial = (double *)xcalloc(nv, sizeof(double));

    int64_t * d = (int64_t *)xmalloc(nv * sizeof(int64_t));
    for(int64_t i = 0; i < nv; i ++) d[i] = -1;

    int64_t q_front = 1;
    int64_t q_rear = 0;
    q[0] = source;
    d[source] = 0;
    paths[source] = 1;

    while(q_rear != q_front) {
        int64_t v = q[q_rear++];
        int64_t d_next = d[v] + 1;

        STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, v) {

            if(d[STINGER_EDGE_DEST] < 0) {
                d[STINGER_EDGE_DEST]     = d_next;
                paths[STINGER_EDGE_DEST] = paths[v];
                q[q_front++]             = STINGER_EDGE_DEST;

                stinger_int64_fetch_add(found_count + STINGER_EDGE_DEST, 1);
            }

            else if(d[STINGER_EDGE_DEST] == d_next) {
                paths[STINGER_EDGE_DEST] += paths[v];
            }

        } STINGER_FORALL_EDGES_OF_VTX_END();
    }

    /* don't process source */
    while(q_front > 1) {
        int64_t w = q[--q_front];

        /* don't maintain parents, do search instead */
        STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, w) {
            if(d[STINGER_EDGE_DEST] == (d[w] - 1)) {
                partial[STINGER_EDGE_DEST] += frac(paths[STINGER_EDGE_DEST],paths[w]) * (1 + partial[w]);
            }
        } STINGER_FORALL_EDGES_OF_VTX_END();
        bc[w] += partial[w];
    }

    free(d);
    free(partial);
    free(paths);
}

void
sample_search(stinger_t * S, int64_t nv, int64_t nsamples, double * bc, int64_t * found_count)
{
    LOG_V_A("  > Beggining with %ld vertices and %ld samples", (long)nv, (long)nsamples);

    int64_t * found = xcalloc(nv, sizeof(int64_t));
    OMP("omp parallel")
    {
        OMP("omp for")
        for(int64_t v = 0; v < nv; v++) {
            found_count[v] = 0;
            bc[v] = 0;
        }

        double * partials = (double *)xcalloc(nv, sizeof(double));

        if(nv < nsamples) {
            int64_t min = nv < nsamples ? nv : nsamples;

            OMP("omp for")
            for(int64_t s = 0; s < min; s++) {
                single_bc_search(S, nv, s, partials, found_count);
            }
        } else {
            OMP("omp for")
            for(int64_t s = 0; s < nsamples; s++) {
                int64_t v = 0;

                while(1) {
                    v = rand() % nv;
                    if(found[v] == 0 && (0 == stinger_int64_fetch_add(found + v, 1))) {
                        break;
                    }
                }

                single_bc_search(S, nv, v, partials, found_count);
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
}

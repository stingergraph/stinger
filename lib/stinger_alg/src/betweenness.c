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
single_bc_search(stinger_t * S, int64_t nv, int64_t source, double * bc, int64_t * found_count)
{

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

            STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, v) {
                //LOG_D_A("(%ld) Begin - d[%ld]=%ld paths[%ld]=%ld",source,STINGER_EDGE_DEST,d[STINGER_EDGE_DEST],STINGER_EDGE_DEST,paths[STINGER_EDGE_DEST]);
                if(d[STINGER_EDGE_DEST] < 0) {
                    d[STINGER_EDGE_DEST]     = d_next;
                    paths[STINGER_EDGE_DEST] = paths[v];
                    count = count + 1;
                    bfs_stack[stack_top++]           = STINGER_EDGE_DEST;

                    stinger_int64_fetch_add(found_count + STINGER_EDGE_DEST, 1);
                }
                else if(d[STINGER_EDGE_DEST] == d_next) {
                    paths[STINGER_EDGE_DEST] += paths[v];
                }
                //LOG_D_A("(%ld) End - d[%ld]=%ld paths[%ld]=%ld",source,STINGER_EDGE_DEST,d[STINGER_EDGE_DEST],STINGER_EDGE_DEST,paths[STINGER_EDGE_DEST]);

            } STINGER_FORALL_OUT_EDGES_OF_VTX_END();

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
            STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, w) {
                if(d[w] == (d[STINGER_EDGE_DEST] - 1)) {
                    dsw += frac(sw,paths[STINGER_EDGE_DEST]) * (1.0 + partial[STINGER_EDGE_DEST]);
                }
            } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
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
}

#if 0
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

    //fprintf(stderr,"Undirected\n");

    while(q_rear != q_front) {
        int64_t v = q[q_rear++];
        int64_t d_next = d[v] + 1;

        //fprintf(stderr,"(%ld) v=%ld d_next=%ld\n",source,v,d_next);

        STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, v) {
            //fprintf(stderr,"(%ld) Begin - d[%ld]=%ld paths[%ld]=%ld\n",source,STINGER_EDGE_DEST,d[STINGER_EDGE_DEST],STINGER_EDGE_DEST,paths[STINGER_EDGE_DEST]);
            if(d[STINGER_EDGE_DEST] < 0) {
                d[STINGER_EDGE_DEST]     = d_next;
                paths[STINGER_EDGE_DEST] = paths[v];
                q[q_front++]             = STINGER_EDGE_DEST;

                stinger_int64_fetch_add(found_count + STINGER_EDGE_DEST, 1);
            }

            else if(d[STINGER_EDGE_DEST] == d_next) {
                paths[STINGER_EDGE_DEST] += paths[v];
            }
            //fprintf(stderr,"(%ld) End - d[%ld]=%ld paths[%ld]=%ld\n",source,STINGER_EDGE_DEST,d[STINGER_EDGE_DEST],STINGER_EDGE_DEST,paths[STINGER_EDGE_DEST]);

        } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
    }

    /* don't process source */
    while(q_front > 1) {
        int64_t w = q[--q_front];

        /* don't maintain parents, do search instead */
        STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, w) {
            if(d[STINGER_EDGE_DEST] == (d[w] - 1)) {
                partial[STINGER_EDGE_DEST] += frac(paths[STINGER_EDGE_DEST],paths[w]) * (1 + partial[w]);
            }
        } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
        bc[w] += partial[w];
        //fprintf(stderr,"(%ld) Sum Partials for %ld -- partial[%ld]=%lf bc[%ld]=%lf\n",source,w,w,partial[w],w,bc[w]);
    }

    xfree(d);
    xfree(partial);
    xfree(paths);
}
#endif

void
sample_search(stinger_t * S, int64_t nv, int64_t nsamples, double * bc, int64_t * found_count)
{
    LOG_V_A("  > Beginning with %ld vertices and %ld samples\n", (long)nv, (long)nsamples);

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
            int64_t min = nv;

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

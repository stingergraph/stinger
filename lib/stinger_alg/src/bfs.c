#include "bfs.h"

#include "stinger_core/x86_full_empty.h"

int64_t
parallel_breadth_first_search (struct stinger * S, int64_t nv,
                            int64_t source, int64_t * marks,
                    int64_t * queue, int64_t * Qhead, int64_t * level)
{
    for (int64_t i = 0; i < nv; i++) {
        level[i] = -1;
        marks[i] = 0;
    }

    int64_t nQ, Qnext, Qstart, Qend;
    /* initialize */
    queue[0] = source;
    level[source] = 0;
    marks[source] = 1;
    Qnext = 1;    /* next open slot in the queue */
    nQ = 1;         /* level we are currently processing */
    Qhead[0] = 0;    /* beginning of the current frontier */
    Qhead[1] = 1;    /* end of the current frontier */

    Qstart = Qhead[nQ-1];
    Qend = Qhead[nQ];

    while (Qstart != Qend) {
        OMP ("omp parallel for")
        for (int64_t j = Qstart; j < Qend; j++) {
            STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN (S, queue[j]) {
                int64_t d = level[STINGER_EDGE_DEST];
                if (d < 0) {
                    if (stinger_int64_fetch_add (&marks[STINGER_EDGE_DEST], 1) == 0) {
                        level[STINGER_EDGE_DEST] = nQ;

                        int64_t mine = stinger_int64_fetch_add(&Qnext, 1);
                        queue[mine] = STINGER_EDGE_DEST;
                    }
                }
            } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
        }

        Qstart = Qhead[nQ-1];
        Qend = Qnext;
        Qhead[nQ++] = Qend;
    }

    return nQ;
}

int64_t
direction_optimizing_parallel_breadth_first_search (struct stinger * S, int64_t nv,
                      int64_t source, int64_t * marks,
                      int64_t * queue, int64_t * Qhead, int64_t * level)
{
    for (int64_t i = 0; i < nv; i++) {
        level[i] = -1;
        marks[i] = 0;
    }

    int64_t nQ, Qnext, Qstart, Qend;
    int64_t mf = 0;  /* number of edges incident on frontier */
    int64_t nf = 0;  /* number of vertices in frontier */
    int64_t mu = 0;  /* number of edges incident on unexplored vertices */
    int64_t nf_last = 0;  /* size of previous frontier */

    /* initialize */
    queue[0] = source;
    level[source] = 0;
    marks[source] = 1;
    Qnext = 1;  /* next open slot in the queue */
    nQ = 1;     /* level we are currently processing */
    Qhead[0] = 0;  /* beginning of the current frontier */
    Qhead[1] = 1;  /* end of the current frontier */

    Qstart = Qhead[nQ-1];
    Qend = Qhead[nQ];

    mf = 0;
    nf = 0;
    mu = stinger_total_edges(S);
    //printf("mf: %ld, nf: %ld, mu: %ld, nf_last: %ld\n", mf, nf, mu, nf_last);

    int64_t top_down = 1;

    while (Qstart != Qend) {
        if (top_down) {
            /* forward (top down) traversal */
            OMP ("omp parallel for")
            for (int64_t j = Qstart; j < Qend; j++) {
                STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN (S, queue[j]) {
                    int64_t d = level[STINGER_EDGE_DEST];
                    if (d < 0) {
                        if (stinger_int64_fetch_add (&marks[STINGER_EDGE_DEST], 1) == 0) {
                            level[STINGER_EDGE_DEST] = nQ;

                            int64_t mine = stinger_int64_fetch_add(&Qnext, 1);
                            queue[mine] = STINGER_EDGE_DEST;
                        }
                    }
                } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
            }
        } else {
            /* reverse (bottom up) traversal */
            OMP ("omp parallel for")
            for (int64_t i = 0; i < nv; i++) {
                int64_t done = 0;
                /* only process unvisited vertices */
                if (!marks[i]) {
                    STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN (S, i) {
                        /* neighbor has been visited */
                        if (!done && level[STINGER_EDGE_DEST] == nQ-1) {
                            level[i] = nQ;
                            marks[i] = 1;

                            /* if we don't queue here, we cannot restart the forward search */
                            int64_t mine = stinger_int64_fetch_add(&Qnext, 1);
                            queue[mine] = i;
                            done = 1;
                        }
                    } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
                }
            }
        }

        Qstart = Qhead[nQ-1];
        Qend = Qnext;
        Qhead[nQ++] = Qend;

        /* calculate new transition parameters */
        nf_last = nf;
        nf = Qend - Qstart;
        mf = 0;
        OMP ("omp parallel for reduction(+:mf) reduction(-:mu)")
        for (int64_t j = Qstart; j < Qend; j++) {
            int64_t deg_j = stinger_outdegree_get(S, queue[j]);
            mf += deg_j;
            mu -= deg_j;
        }
        //printf("mf: %ld, nf: %ld, mu: %ld, nf_last: %ld\n", mf, nf, mu, nf_last);

        /* evaluate the transition state machine */
        if (top_down) {
            if (mf > (mu / 14) && (nf - nf_last > 0)) {
                top_down = 0;
            }
        } else {
            if (nf < (nv / 24) && (nf - nf_last < 0)) {
                top_down = 1;
            }
        }
    } /* end while */

    return nQ;
}

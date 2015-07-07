/* -*- mode: C; mode: folding; fill-column: 70; -*- */
#define _XOPEN_SOURCE 600
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include "stinger_core/stinger_atomics.h"
#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"

#include "stinger_utils/stinger_utils.h"
#include "stinger_utils/timer.h"

#include "bfs.h"

#define INC 1


static int64_t nv, ne, naction;
static int64_t *restrict off;
static int64_t *restrict from;
static int64_t *restrict ind;
static int64_t *restrict weight;
static int64_t *restrict action;

/* handles for I/O memory */
static int64_t *restrict graphmem;
static int64_t *restrict actionmem;

static char *initial_graph_name = INITIAL_GRAPH_NAME_DEFAULT;
static char *action_stream_name = ACTION_STREAM_NAME_DEFAULT;

static long batch_size = BATCH_SIZE_DEFAULT;
static long nbatch = NBATCH_DEFAULT;

static struct stinger *S;


int
main (const int argc, char *argv[])
{
  parse_args (argc, argv, &initial_graph_name, &action_stream_name,
              &batch_size, &nbatch);
  STATS_INIT ();

  load_graph_and_action_stream (initial_graph_name, &nv, &ne,
                                (int64_t **) & off, (int64_t **) & ind,
                                (int64_t **) & weight,
                                (int64_t **) & graphmem, action_stream_name,
                                &naction, (int64_t **) & action,
                                (int64_t **) & actionmem);

  print_initial_graph_stats (nv, ne, batch_size, nbatch, naction);
  BATCH_SIZE_CHECK ();

  /* Convert to STINGER */
  tic ();
  S = stinger_new ();
  stinger_set_initial_edges (S, nv, 0, off, ind, weight, NULL, NULL, 0);
  PRINT_STAT_DOUBLE ("time_stinger", toc ());
  fflush (stdout);

  int64_t numSteps = 3;
  int64_t src_dest_pair[2] = { 124, 381 };

  tic ();
  int64_t size_intersect =
    st_conn_stinger (S, nv, ne, src_dest_pair, 1, numSteps);
  PRINT_STAT_DOUBLE ("time_st_conn_stinger", toc ());
  PRINT_STAT_INT64 ("size_intersect", size_intersect);

  stinger_free_all (S);
  free (graphmem);
  free (actionmem);
  STATS_END ();
}


void
bfs_stinger (const struct stinger *G, const int64_t nv, const int64_t ne,
             const int64_t startV,
             int64_t * marks, const int64_t numSteps,
             int64_t * Q, int64_t * QHead, int64_t * neighbors)
{
  int64_t j, k, s;
  int64_t nQ, Qnext, Qstart, Qend;
  int64_t w_start;


  MTA ("mta assert nodep")
    for (j = 0; j < nv; j++) {
      marks[j] = 0;
    }

  s = startV;
  /* Push node s onto Q and set bounds for first Q sublist */
  Q[0] = s;
  Qnext = 1;
  nQ = 1;
  QHead[0] = 0;
  QHead[1] = 1;
  marks[s] = 1;

 PushOnStack:                   /* Push nodes onto Q */

  /* Execute the nested loop for each node v on the Q AND
     for each neighbor w of v  */
  Qstart = QHead[nQ - 1];
  Qend = QHead[nQ];
  w_start = 0;

  MTA ("mta assert no dependence")
    MTA ("mta block dynamic schedule")
    for (j = Qstart; j < Qend; j++) {
      int64_t v = Q[j];
      size_t d;
      size_t deg_v = stinger_outdegree (G, v);
      int64_t my_w = stinger_int64_fetch_add (&w_start, deg_v);
      stinger_gather_typed_successors (G, 0, v, &d, &neighbors[my_w], deg_v);
      assert (d == deg_v);

      MTA ("mta assert nodep")
        for (k = 0; k < deg_v; k++) {
          int64_t d, w, l;
          w = neighbors[my_w + k];
          /* If node has not been visited, set distance and push on Q */
          if (stinger_int64_fetch_add (marks + w, 1) == 0) {
            Q[stinger_int64_fetch_add (&Qnext, 1)] = w;
          }
        }
    }

  if (Qnext != QHead[nQ] && nQ < numSteps) {
    nQ++;
    QHead[nQ] = Qnext;
    goto PushOnStack;
  }
}


int64_t
st_conn_stinger (const struct stinger *G, const int64_t nv, const int64_t ne,
                 const int64_t * sources, const int64_t num,
                 const int64_t numSteps)
{
  int64_t k, x;

  int64_t *Q_big = (int64_t *) xmalloc (INC * nv * sizeof (int64_t));
  int64_t *marks_s_big = (int64_t *) xmalloc (INC * nv * sizeof (int64_t));
  int64_t *marks_t_big = (int64_t *) xmalloc (INC * nv * sizeof (int64_t));
  int64_t *QHead_big =
    (int64_t *) xmalloc (INC * 2 * numSteps * sizeof (int64_t));
  int64_t *neighbors_big = (int64_t *) xmalloc (INC * ne * sizeof (int64_t));

  int64_t count = 0;

  k = 0;
  x = 0;

  OMP ("omp parallel for")
    MTA ("mta assert parallel")
    MTA ("mta loop future")
    MTA ("mta assert nodep")
    MTA ("mta assert no alias")
    for (x = 0; x < INC; x++) {
      int64_t *Q = Q_big + x * nv;
      int64_t *marks_s = marks_s_big + x * nv;
      int64_t *marks_t = marks_t_big + x * nv;
      int64_t *QHead = QHead_big + x * 2 * numSteps;
      int64_t *neighbors = neighbors_big + x * ne;

      for (int64_t claimedk = stinger_int64_fetch_add (&k, 2);
           claimedk < 2 * num; claimedk = stinger_int64_fetch_add (&k, 2)) {
        int64_t s = sources[claimedk];
        int64_t deg_s = stinger_outdegree (G, s);
        int64_t t = sources[claimedk + 1];
        int64_t deg_t = stinger_outdegree (G, t);

        if (deg_s == 0 || deg_t == 0) {
          stinger_int64_fetch_add (&count, 1);
        } else {
          bfs_stinger (G, nv, ne, s, marks_s, numSteps, Q, QHead, neighbors);
          bfs_stinger (G, nv, ne, t, marks_t, numSteps, Q, QHead, neighbors);
          int64_t local_count = 0;

          MTA ("mta assert nodep")
            for (int64_t j = 0; j < nv; j++) {
              if (marks_s[j] && marks_t[j])
                stinger_int64_fetch_add (&local_count, 1);
            }

          if (local_count == 0)
            stinger_int64_fetch_add (&count, 1);
        }
      }
    }

  free (neighbors_big);
  free (QHead_big);
  free (marks_t_big);
  free (marks_s_big);
  free (Q_big);

  return count;

}


int64_t
st_conn_stinger_source (const struct stinger * G, const int64_t nv,
                        const int64_t ne, const int64_t from,
                        const int64_t * sources, const int64_t num,
                        const int64_t numSteps)
{
  int64_t k;

  int64_t *Q = (int64_t *) xmalloc (nv * sizeof (int64_t));
  int64_t *marks_s = (int64_t *) xmalloc (nv * sizeof (int64_t));
  int64_t *marks_t = (int64_t *) xmalloc (nv * sizeof (int64_t));
  int64_t *QHead = (int64_t *) xmalloc (2 * numSteps * sizeof (int64_t));
  int64_t *neighbors = (int64_t *) xmalloc (ne * sizeof (int64_t));

  int64_t count = 0;

  int64_t deg_s = stinger_outdegree (G, from);
  if (deg_s == 0) {
    free (neighbors);
    free (QHead);
    free (marks_t);
    free (marks_s);
    free (Q);
    return num;
  }

  bfs_stinger (G, nv, ne, from, marks_s, numSteps, Q, QHead, neighbors);

  for (k = 0; k < num; k++) {
    int64_t t = sources[k];
    int64_t deg_t = stinger_outdegree (G, t);

    if (deg_t == 0) {
      stinger_int64_fetch_add (&count, 1);
    } else {
      bfs_stinger (G, nv, ne, t, marks_t, numSteps, Q, QHead, neighbors);

      int64_t local_count = 0;

      MTA ("mta assert nodep")
        for (int64_t j = 0; j < nv; j++) {
          if (marks_s[j] && marks_t[j])
            stinger_int64_fetch_add (&local_count, 1);
        }

      if (local_count == 0)
        stinger_int64_fetch_add (&count, 1);
    }
  }

  free (neighbors);
  free (QHead);
  free (marks_t);
  free (marks_s);
  free (Q);

  return count;

}


void
bfs_csr (const int64_t nv, const int64_t ne, const int64_t * off,
         const int64_t * ind, const int64_t startV, int64_t * marks,
         const int64_t numSteps, int64_t * Q, int64_t * QHead)
{
  int64_t j, k, s;
  int64_t nQ, Qnext, Qstart, Qend;


  MTA ("mta assert nodep")
    for (j = 0; j < nv; j++) {
      marks[j] = 0;
    }

  s = startV;
  /* Push node s onto Q and set bounds for first Q sublist */
  Q[0] = s;
  Qnext = 1;
  nQ = 1;
  QHead[0] = 0;
  QHead[1] = 1;
  marks[s] = 1;

 PushOnStack:                   /* Push nodes onto Q */

  /* Execute the nested loop for each node v on the Q AND
     for each neighbor w of v  */
  Qstart = QHead[nQ - 1];
  Qend = QHead[nQ];

  MTA ("mta assert no dependence")
    MTA ("mta block dynamic schedule")
    for (j = Qstart; j < Qend; j++) {
      int64_t v = Q[j];
      int64_t myStart = off[v];
      int64_t myEnd = off[v + 1];

      MTA ("mta assert nodep")
        for (k = myStart; k < myEnd; k++) {
          int64_t d, w, l;
          w = ind[k];
          /* If node has not been visited, set distance and push on Q */
          if (stinger_int64_fetch_add (marks + w, 1) == 0) {
            Q[stinger_int64_fetch_add (&Qnext, 1)] = w;
          }
        }
    }

  if (Qnext != QHead[nQ] && nQ < numSteps) {
    nQ++;
    QHead[nQ] = Qnext;
    goto PushOnStack;
  }
}


int64_t
st_conn_csr (const int64_t nv, const int64_t ne, const int64_t * off,
             const int64_t * ind, const int64_t * sources, const int64_t num,
             const int64_t numSteps)
{
  int64_t k, x;

  int64_t *Q_big = (int64_t *) xmalloc (INC * nv * sizeof (int64_t));
  int64_t *marks_s_big = (int64_t *) xmalloc (INC * nv * sizeof (int64_t));
  int64_t *marks_t_big = (int64_t *) xmalloc (INC * nv * sizeof (int64_t));
  int64_t *QHead_big =
    (int64_t *) xmalloc (INC * 2 * numSteps * sizeof (int64_t));
  int64_t *neighbors_big = (int64_t *) xmalloc (INC * ne * sizeof (int64_t));

  int64_t count = 0;

  k = 0;

  MTA ("mta assert parallel")
    MTA ("mta loop future")
    for (x = 0; x < INC; x++) {
      int64_t *Q = Q_big + x * nv;
      int64_t *marks_s = marks_s_big + x * nv;
      int64_t *marks_t = marks_t_big + x * nv;
      int64_t *QHead = QHead_big + x * 2 * numSteps;
      int64_t *neighbors = neighbors_big + x * ne;

      for (int64_t claimedk = stinger_int64_fetch_add (&k, 2);
           claimedk < 2 * num; claimedk = stinger_int64_fetch_add (&k, 2)) {

        int64_t s = sources[claimedk];
        int64_t t = sources[claimedk + 1];
        bfs_csr (nv, ne, off, ind, s, marks_s, numSteps, Q, QHead);
        bfs_csr (nv, ne, off, ind, t, marks_t, numSteps, Q, QHead);

        int64_t local_count = 0;

        MTA ("mta assert nodep")
          for (int64_t j = 0; j < nv; j++) {
            if (marks_s[j] && marks_t[j])
              stinger_int64_fetch_add (&local_count, 1);
          }

        if (local_count == 0)
          stinger_int64_fetch_add (&count, 1);
      }
    }

  free (neighbors_big);
  free (QHead_big);
  free (marks_t_big);
  free (marks_s_big);
  free (Q_big);

  return count;

}

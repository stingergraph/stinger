/* -*- mode: C; mode: folding; fill-column: 70; -*- */
#define _XOPEN_SOURCE 600
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include "stinger_core/stinger_atomics.h"
#include "stinger_utils/stinger_utils.h"
#include "stinger_core/stinger.h"
#include "stinger_utils/timer.h"
#include "stinger_core/xmalloc.h"
#include "connected-comp.h"

#define N 500000


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

  int64_t *component_map = xmalloc (nv * sizeof (int64_t));

  /* Convert to STINGER */
  tic ();
  S = stinger_new ();
  stinger_set_initial_edges (S, nv, 0, off, ind, weight, NULL, NULL, 0);
  PRINT_STAT_DOUBLE ("time_stinger", toc ());
  fflush (stdout);

  tic ();
  int64_t num_comp_end =
    connected_components_stinger (S, nv, ne, component_map, NULL, NULL, NULL,
                                  NULL, NULL);
  PRINT_STAT_DOUBLE ("time_components_tree", toc ());
  PRINT_STAT_INT64 ("number_of_components", num_comp_end);

  stinger_free_all (S);
  free (graphmem);
  free (actionmem);
  STATS_END ();
}


int64_t
connected_components (const size_t nv, const size_t ne,
                      const int64_t * restrict off,
                      const int64_t * restrict ind, int64_t * restrict d,
                      int64_t * restrict crossV, int64_t * restrict crossU,
                      int64_t * restrict marks, int64_t * restrict T)
{
  int64_t i, change, k = 0, count = 0, cross_count = 0;
  int64_t *stackstore = NULL;

  int64_t freeV = 0;
  int64_t freeU = 0;
  int64_t freemarks = 0;
  int64_t freeT = 0;

  if (crossV == NULL) {
    crossV = xmalloc (2 * ne * sizeof (int64_t));
    freeV = 1;
  }
  if (crossU == NULL) {
    crossU = xmalloc (2 * ne * sizeof (int64_t));
    freeU = 1;
  }
  if (marks == NULL) {
    marks = xmalloc (nv * sizeof (int64_t));
    freemarks = 1;
  }
  if (T == NULL) {
    T = xmalloc (nv * sizeof (int64_t));
    freeT = 1;
  }

#if !defined(__MTA__)
  stackstore = xmalloc (nv * sizeof (int64_t));
#endif

  MTA ("mta assert nodep")
    for (i = 0; i < nv; i++) {
      d[i] = i;
      marks[i] = 0;
    }

  MTA ("mta assert nodep")
    for (i = 0; i < nv; i++) {
#if !defined(__MTA__)
      int64_t *stack = stackstore;
#else
      int64_t stack[N];
#endif
      int64_t my_root;
      int64_t top = -1;
      int64_t v, u;

      if (stinger_int64_fetch_add (marks + i, 1) == 0) {
        top = -1;
        my_root = i;
        push (my_root, stack, &top);
        while (!is_empty (stack, &top)) {
          int64_t myStart, myEnd;
          int64_t k;
          u = pop (stack, &top);
          myStart = off[u];
          myEnd = off[u + 1];

          for (k = myStart; k < myEnd; k++) {
            v = ind[k];
            assert (v < nv);
            if (stinger_int64_fetch_add (marks + v, 1) == 0) {
              d[v] = my_root;
              push (v, stack, &top);
            } else {
              if (!(d[v] == d[my_root])) {
                int64_t t = stinger_int64_fetch_add (&cross_count, 1);
                crossU[t] = u;
                crossV[t] = v;
              }
            }
          }
        }
      }
    }

  change = 1;
  k = 0;
  while (change) {
    change = 0;
    k++;

    MTA ("mta assert nodep")
      for (i = 0; i < cross_count; i++) {
        int64_t u = crossU[i];
        int64_t v = crossV[i];
        int64_t tmp;
        if (d[u] < d[v] && d[d[v]] == d[v]) {
          d[d[v]] = d[u];
          change = 1;
        }
        tmp = u;
        u = v;
        v = tmp;
        if (d[u] < d[v] && d[d[v]] == d[v]) {
          d[d[v]] = d[u];
          change = 1;
        }
      }

    if (change == 0)
      break;

    MTA ("mta assert nodep");
    for (i = 0; i < nv; i++) {
      while (d[i] != d[d[i]])
        d[i] = d[d[i]];
    }
  }

  for (i = 0; i < nv; i++) {
    T[i] = 0;
  }

  MTA ("mta assert nodep")
    for (i = 0; i < nv; i++) {
      int64_t temp = 0;
      if (stinger_int64_fetch_add (T + d[i], 1) == 0) {
        temp = 1;
      }
      count += temp;
    }

#if !defined(__MTA__)
  free (stackstore);
#endif
  if (freeT)
    free (T);
  if (freemarks)
    free (marks);
  if (freeU)
    free (crossU);
  if (freeV)
    free (crossV);

  return count;

}

int
spanning_tree_is_good (int64_t * restrict d, int64_t * restrict tree,
                       int64_t nv)
{
  int rtn = 0;
  OMP ("omp parallel for reduction(+:rtn)")
    for (uint64_t i = 0; i < nv; i++) {
      uint64_t j = i;
      while (j != tree[j])
        j = tree[tree[j]];
      if (j != d[i])
        rtn++;
    }
  return rtn == 0;
}

int64_t
connected_components_edge (const size_t nv, const size_t ne,
                           const int64_t * restrict sV,
                           const int64_t * restrict eV, int64_t * restrict D)
{
  int64_t count = 0;
  int64_t nchanged;
  int64_t freemarks = 0;

  if (D == NULL) {
    D = xmalloc (nv * sizeof (int64_t));
    freemarks = 1;
  }

  OMP ("omp parallel") {
    while (1) {
      OMP ("omp single") nchanged = 0;
      MTA ("mta assert nodep") OMP ("omp for reduction(+:nchanged)")
        for (int64_t k = 0; k < ne; ++k) {
          const int64_t i = sV[k];
          const int64_t j = eV[k];
          if (D[i] < D[j]) {
            D[D[j]] = D[i];
            ++nchanged;
          }
        }
      if (!nchanged)
        break;
      MTA ("mta assert nodep") OMP ("omp for")
        for (int64_t i = 0; i < nv; ++i)
          while (D[i] != D[D[i]])
            D[i] = D[D[i]];
    }

    MTA ("mta assert nodep") OMP ("omp for reduction(+:count)")
      for (int64_t i = 0; i < nv; ++i) {
        while (D[i] != D[D[i]])
          D[i] = D[D[i]];
        if (D[i] == i)
          ++count;
      }
  }

  if (freemarks)
    free (D);

  return count;
}


void
component_dist (const size_t nv, int64_t * restrict d,
                int64_t * restrict cardinality, const int64_t numColors)
{
  uint64_t sum = 0, sum_sq = 0;
  double average, avg_sq, variance, std_dev;
  int64_t i;

  uint64_t max = 0;

  OMP ("omp parallel for") MTA ("mta assert nodep")
    for (i = 0; i < nv; i++) {
      cardinality[i] = 0;
    }

  for (i = 0; i < nv; i++) {
    cardinality[d[i]]++;
  }

  for (i = 0; i < nv; i++) {
    uint64_t degree = cardinality[i];
    sum_sq += degree * degree;
    sum += degree;

    if (degree > max) {
      max = degree;
    }
  }

  uint64_t max2 = 0;
  uint64_t checker = max;

  MTA ("mta assert nodep")
    for (i = 0; i < nv; i++) {
      uint64_t degree = cardinality[i];
      uint64_t tmp = (checker == degree ? i : 0);
      if (tmp > max2)
        max2 = tmp;
    }

  average = (double) sum / numColors;
  avg_sq = (double) sum_sq / numColors;
  variance = avg_sq - (average * average);
  std_dev = sqrt (variance);

  printf ("\t\"max_num_vertices\": %ld,\n", max);
  printf ("\t\"avg_num_vertices\": %20.15e,\n", average);
  printf ("\t\"exp_value_x2\": %20.15e,\n", avg_sq);
  printf ("\t\"variance\": %20.15e,\n", variance);
  printf ("\t\"std_dev\": %20.15e,\n\n", std_dev);

}

int64_t
connected_components_stinger (const struct stinger *S, const size_t nv,
                              const size_t ne, int64_t * restrict d,
                              int64_t * restrict crossV,
                              int64_t * restrict crossU,
                              int64_t * restrict marks, int64_t * restrict T,
                              int64_t * restrict neighbors)
{
  int64_t i, change, k = 0, count = 0, cross_count = 0;
  int64_t *stackstore = NULL;

  int64_t freeV = 0;
  int64_t freeU = 0;
  int64_t freemarks = 0;
  int64_t freeT = 0;
  int64_t freeneighbors = 0;

  if (crossV == NULL) {
    crossV = xmalloc (2 * ne * sizeof (int64_t));
    freeV = 1;
  }
  if (crossU == NULL) {
    crossU = xmalloc (2 * ne * sizeof (int64_t));
    freeU = 1;
  }
  if (marks == NULL) {
    marks = xmalloc (nv * sizeof (int64_t));
    freemarks = 1;
  }
  if (T == NULL) {
    T = xmalloc (nv * sizeof (int64_t));
    freeT = 1;
  }
  if (neighbors == NULL) {
    neighbors = xmalloc (2 * ne * sizeof (int64_t));
    freeneighbors = 1;
  }

#if !defined(__MTA__)
  stackstore = xmalloc (nv * sizeof (int64_t));
#endif

  MTA ("mta assert nodep")
    for (i = 0; i < nv; i++) {
      d[i] = i;
      marks[i] = 0;
    }

  int64_t head = 0;

  MTA ("mta assert nodep")
    for (i = 0; i < nv; i++) {
#if !defined(__MTA__)
      int64_t *stack = stackstore;
#else
      int64_t stack[N];
#endif
      int64_t my_root;
      int64_t top = -1;
      int64_t v, u;
      int64_t deg_u;

      if (stinger_int64_fetch_add (marks + i, 1) == 0) {
        top = -1;
        my_root = i;
        push (my_root, stack, &top);
        while (!is_empty (stack, &top)) {
          int64_t myStart, myEnd;
          int64_t k;
          size_t md;
          u = pop (stack, &top);
          deg_u = stinger_outdegree (S, u);
          myStart = stinger_int64_fetch_add (&head, deg_u);
          myEnd = myStart + deg_u;
          stinger_gather_typed_successors (S, 0, u, &md,
                                           &neighbors[myStart], deg_u);

          for (k = myStart; k < myEnd; k++) {
            v = neighbors[k];
            if (stinger_int64_fetch_add (marks + v, 1) == 0) {
              d[v] = my_root;
              push (v, stack, &top);
            } else {
              if (!(d[v] == d[my_root])) {
                int64_t t = stinger_int64_fetch_add (&cross_count, 1);
                crossU[t] = u;
                crossV[t] = v;
              }
            }
          }
        }
      }
    }

  change = 1;
  k = 0;
  while (change) {
    change = 0;
    k++;

    MTA ("mta assert nodep")
      for (i = 0; i < cross_count; i++) {
        int64_t u = crossU[i];
        int64_t v = crossV[i];
        int64_t tmp;
        if (d[u] < d[v] && d[d[v]] == d[v]) {
          d[d[v]] = d[u];
          change = 1;
        }
        tmp = u;
        u = v;
        v = tmp;
        if (d[u] < d[v] && d[d[v]] == d[v]) {
          d[d[v]] = d[u];
          change = 1;
        }
      }

    if (change == 0)
      break;

    MTA ("mta assert nodep");
    for (i = 0; i < nv; i++) {
      while (d[i] != d[d[i]])
        d[i] = d[d[i]];
    }
  }

  for (i = 0; i < nv; i++) {
    T[i] = 0;
  }

  MTA ("mta assert nodep")
    for (i = 0; i < nv; i++) {
      int64_t temp = 0;
      if (stinger_int64_fetch_add (T + d[i], 1) == 0) {
        temp = 1;
      }
      count += temp;
    }

#if !defined(__MTA__)
  free (stackstore);
#endif
  if (freeneighbors)
    free (neighbors);
  if (freeT)
    free (T);
  if (freemarks)
    free (marks);
  if (freeU)
    free (crossU);
  if (freeV)
    free (crossV);

  return count;

}

static void
push (int64_t a, int64_t * stack, int64_t * top)
{
  (*top)++;
  stack[*top] = a;
}

static int64_t
pop (int64_t * stack, int64_t * top)
{
  int a = stack[*top];
  (*top)--;
  return a;
}

static int64_t
is_empty (int64_t * stack, int64_t * top)
{
  if ((*top) == -1)
    return 1;
  else
    return 0;
}

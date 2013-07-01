/* -*- mode: C; mode: folding; fill-column: 70; -*- */
#define _XOPEN_SOURCE 600
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include "connected-comp.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_utils/stinger_utils.h"
#include "stinger_core/stinger.h"
#include "stinger_utils/timer.h"
#include "stinger_core/xmalloc.h"

/* Note: the bit array code shifts small batch sizes to zero.
   Do not enable until fixed. */
/* #define USE_BIT_ARRAY 1 */
/* #define SPAN_TREE_STATS 1 */
//#define CHECK_ACCURACY 1


static char * initial_graph_name = INITIAL_GRAPH_NAME_DEFAULT;
static char * action_stream_name = ACTION_STREAM_NAME_DEFAULT;

static long batch_size = BATCH_SIZE_DEFAULT;
static long nbatch = NBATCH_DEFAULT;

static int64_t nv, ne, naction;
static int64_t * restrict off;
static int64_t * restrict from;
static int64_t * restrict ind;
static int64_t * restrict weight;
static int64_t * restrict action;

/* handles for I/O memory */
static int64_t * restrict graphmem;
static int64_t * restrict actionmem;

static struct stinger * S;

static int64_t num_comp, num_comp_init, num_comp_end;
static int64_t * restrict ncomp;
static int64_t * restrict ncomp_init;
static int64_t * restrict ncomp_end;
static int64_t * restrict ncompsize;
static int64_t * restrict ncompsize_init;
static int64_t * restrict ncompsize_end;

static int64_t * restrict tree;

static double ncomp_time, stinger_time;

static int64_t * restrict m1;
static int64_t * restrict m2;

static size_t max_ntrace;
static double *update_time_trace;


static int
i64_pair_cmp (const void *pa, const void *pb)
{
  const int64_t ai = ((const int64_t *) pa)[0];
  const int64_t bi = ((const int64_t *) pb)[0];
  if (ai < bi)
    return -1;
  if (ai > bi)
    return 1;
  const int64_t aj = ((const int64_t *) pa)[1];
  const int64_t bj = ((const int64_t *) pb)[1];
  if (aj < bj)
    return -1;
  if (aj > bj)
    return 1;
  return 0;
}

int
main (const int argc, char *argv[])
{
  parse_args (argc, argv, &initial_graph_name, &action_stream_name, &batch_size, &nbatch);
  STATS_INIT();

  tic ();
  load_graph_and_action_stream (initial_graph_name, &nv, &ne, (int64_t**)&off,
                                (int64_t**)&ind, (int64_t**)&weight, (int64_t**)&graphmem,
                                action_stream_name, &naction, (int64_t**)&action, (int64_t**)&actionmem);

  print_initial_graph_stats (nv, ne, batch_size, nbatch, naction);
  BATCH_SIZE_CHECK();

  ncomp = xcalloc (4*nv, sizeof(*ncomp));
  ncomp_end = &ncomp[nv];
  ncomp_init = &ncomp_end[nv];
  tree = &ncomp_init[nv];

  ncompsize = xcalloc (3*nv, sizeof(*ncompsize));
  ncompsize_end = &ncompsize[nv];
  ncompsize_init = &ncompsize_end[nv];

#if USE_BIT_ARRAY
  int64_t zeros, ones, twos;
  int64_t num_bit_arrays = batch_size >> 4;
  int64_t *bitarray_off = xmalloc (nv * sizeof(int64_t));
  int64_t *bitarray[num_bit_arrays];
  /* Not a sufficient fix: if (num_bit_arrays < 1) num_bit_arrays = 1; */

  for(int64_t k = 0; k < num_bit_arrays; ++k) {
    bitarray[k] = xmalloc(sizeof(int64_t) * (nv>>6));
  }
#endif

  /* activate threads */
  OMP("omp parallel for")
    MTA("mta assert nodep")
    for (int64_t k = 0; k < nv; ++k) {
      ncomp[k] = -1;
      ncompsize[k] = 0;
    }

  /* Connected Components */
  tic ();
  num_comp = connected_components (nv, ne, off, ind, ncomp, NULL, NULL, NULL, NULL);
  ncomp_time = toc ();
  PRINT_STAT_DOUBLE ("time_components", ncomp_time);
  PRINT_STAT_INT64 ("num_components_init", num_comp);

  memcpy (ncomp_init, ncomp, nv * sizeof (*ncomp));
  num_comp_init = num_comp;

  /* Component Distribution Statistics */
  component_dist (nv, ncomp, ncompsize, num_comp);
  memcpy (ncompsize_init, ncompsize, nv * sizeof (*ncompsize));

  max_ntrace = nbatch;
  update_time_trace = xmalloc (max_ntrace * sizeof(*update_time_trace));

  /* Run the updates */
#if !defined(__MTA__)
  memcpy (ncomp, ncomp_init, nv * sizeof (*ncomp));
  memcpy (ncompsize, ncompsize_init, nv * sizeof (*ncompsize));
#else
  MTA("mta use 100 streams")
    for (size_t k = 0; k < nv; ++k) {
      ncomp[k] = ncomp_init[k];
      ncompsize[k] = ncompsize_init[k];
    }
#endif
  num_comp = num_comp_init;

  /* Convert to STINGER */
  tic ();
  S = stinger_new ();
  stinger_set_initial_edges (S, nv, 0, off, ind, weight, NULL, NULL, 0);
  stinger_time = toc ();
  PRINT_STAT_DOUBLE ("time_stinger", stinger_time);

  tic();
  num_comp = connected_components_stinger(S, nv, ne, ncomp, NULL, NULL, NULL, NULL, NULL);
  PRINT_STAT_DOUBLE ("time_components_stinger", toc() );
  PRINT_STAT_INT64 ("number_components", num_comp);

  tic();
  num_comp = connected_components_stinger_edge_parallel_and_tree(nv, ne, S, ncomp, NULL, tree);
  PRINT_STAT_DOUBLE ("time_components_tree", toc() );
  PRINT_STAT_INT64 ("number_components", num_comp);

  if (spanning_tree_is_good(ncomp, tree, nv))
    printf(",\n\t\"tree_correct\": true");
  else
    printf(",\n\t\"tree_correct\": false");


#define ACTI(k) (action[2*(k)])
#define ACTJ(k) (action[2*(k)+1])
#define ACTI2(k) (act[2*(k)])
#define ACTJ2(k) (act[2*(k)+1])

  m1 = xmalloc (4 * batch_size * sizeof(int64_t));
  m2 = m1 + (2 * batch_size);

  int64_t *neighbors = xmalloc(2 * ne * sizeof(int64_t));

  static size_t ntrace = 0;

  /* Batching */
  double t;

  int64_t * crossV = xmalloc (2 * ne * sizeof(int64_t));
  int64_t * crossU = xmalloc (2 * ne * sizeof(int64_t));
  int64_t * marks = xmalloc (nv * sizeof(int64_t));
  int64_t * T = xmalloc (nv * sizeof(int64_t));
  int64_t * start = xmalloc((nv+2)*sizeof(start));
  int64_t * comp_to_merge = xmalloc(nv * sizeof(comp_to_merge));

  int64_t num_remove_phase = 0;
  int64_t * willchange = xmalloc(nv * sizeof(int64_t));

#if SPAN_TREE_STATS
  int64_t deletes_total = 0;
  int64_t deletes_in_tree = 0;
  int64_t deletes_in_tree_caught_bitarrays = 0;
  uint64_t deletes_in_tree_caught_climbing = 0;
#endif

#if CHECK_ACCURACY
  int64_t * tempcomp = xmalloc(nv * sizeof(int64_t));
  int64_t * temptree = xmalloc(nv * sizeof(int64_t));
  uint64_t deletes_that_split = 0;
#endif

  int64_t * act = xmalloc (2 * 2 * batch_size * sizeof(*act));
  int64_t * insoff = xmalloc ((2 * 2 * batch_size + 1) * sizeof(*insoff));
  int64_t * deloff = xmalloc ((2 * 2 * batch_size + 1) * sizeof(*deloff));
  int64_t * actk = xmalloc ((2 * 2 * batch_size + 1) * sizeof(*actk));
  int64_t * delA = xmalloc (2 * batch_size * sizeof(*delA));
  int64_t * delB = xmalloc (2 * batch_size * sizeof(*delB));
  int64_t * delC = xmalloc (2 * batch_size * sizeof(*delC));


  /* for each batch */
  for (int actno = 0; actno < nbatch * batch_size; actno += batch_size) {
    const int endact = (actno + batch_size > naction ? naction : actno + batch_size);
    int nact = 2 * (endact - actno);
    int n = 1;
    int k2 = 0;

    tic ();

#if USE_BIT_ARRAY
    OMP("omp parallel for")
      for (int64_t k = 0; k < nv; k++) {
        marks[k] = 0;
      }
    zeros = 0;
    ones = 0;
#endif

    int64_t N = stinger_sort_actions (endact - actno, &action[2*actno], insoff, deloff, act);

#if USE_BIT_ARRAY
    MTA("mta assert nodep")
      MTA("mta block schedule")
      OMP("omp parallel for")
      for (int k = actno; k < endact; k++) {
        const int64_t i = ACTI(k);
        const int64_t j = ACTJ(k);
        if (i != j) {
#if SPAN_TREE_STATS
          if(i < 0) {
            stinger_int64_fetch_add(&deletes_total, 1);
          }
#endif
          if (i < 0 && (tree[-i-1] == -j-1 || tree[-j-1] == -i-1)) {   // for deletions, create histogram of vertices involved
#if SPAN_TREE_STATS
            stinger_int64_fetch_add(&deletes_in_tree, 1);
#endif
            if (stinger_int64_fetch_add(&marks[-i-1], 1)==0) {
              int64_t where = stinger_int64_fetch_add(&zeros, 1);
              bitarray_off[-i-1] = where;
            }
            if (stinger_int64_fetch_add(&marks[-j-1], 1)==0) {
              int64_t where = stinger_int64_fetch_add(&zeros, 1);
              bitarray_off[-j-1] = where;
            }
            int64_t where = stinger_int64_fetch_add(&ones, 2);
            assert(where < 2*batch_size);
            delA[where] = -i-1;
            delA[where+1] = -j-1;
          }
        }
      }
#endif

    stinger_remove_and_insert_batch (S, 0, actno+1, N, insoff, deloff, act);

#if USE_BIT_ARRAY
    OMP("omp parallel for")
      for (int64_t k = 0; k < nv; k++) {
        marks[k] = 0;
      }

    int64_t head = 0;
    /* Build bit arrays for each vertex with an incident deletion */
    MTA("mta assert nodep")
      OMP("omp parallel for")
      for (int64_t k = 0; k < ones; k+=2) {
        const int64_t i = delA[k];
        if (stinger_int64_fetch_add(&marks[i], 1)==0) {
          int64_t off = bitarray_off[i];
          assert(off < num_bit_arrays);
          size_t d;
          size_t deg_i = stinger_outdegree(S, i);
          int64_t my_w = stinger_int64_fetch_add(&head, deg_i);
          assert (my_w < 2*ne);
          stinger_gather_typed_successors(S, 0, i, &d, &neighbors[my_w], deg_i);
          assert (d == deg_i);

          MTA("mta assert nodep")
            for (int64_t p = 0; p < nv >> 6; p++) {
              bitarray[off][p] = 0;
            }

          MTA("mta assert nodep")
            for (int64_t p = 0; p < deg_i; p++) {
              int64_t w = neighbors[my_w + p];
              int64_t word_offset = w >> 6;
              int64_t bit_offset = w & 0x3Fu;
              int64_t bit = ((uint64_t) 1) << bit_offset;

              MTA("mta update")
                bitarray[off][word_offset] |= bit;
            }
        }
      }

    /* Query time */
    head = 0;
    int64_t no = 0;
    MTA("mta assert nodep")
      OMP("omp parallel for")
      for (int64_t k = 0; k < ones; k+=2) {
        const int64_t i = delA[k];
        const int64_t j = delA[k+1];
        int64_t off = bitarray_off[i];
        size_t d;
        size_t deg_j = stinger_outdegree(S, j);
        int64_t my_w = stinger_int64_fetch_add(&head, deg_j);
        assert (my_w < 2*ne);
        stinger_gather_typed_successors(S, 0, j, &d, &neighbors[my_w], deg_j);
        assert (d == deg_j);

        int64_t sum = 0;
        for (int64_t p = 0; p < deg_j; p++) {
          int64_t w = neighbors[my_w + p];
          int64_t word_offset = w >> 6;
          int64_t bit_offset = w & 0x3Fu;
          int64_t bit = ((uint64_t) 1) << bit_offset;

          int64_t result = bitarray[off][word_offset] & bit;
          sum += result;
          if(result) {
            if(tree[i] == j && tree[w] != i)
              tree[i] = w;
            else if(tree[j] == i && tree[w] != j)
              tree[j] = w;
          }
        }
        if (!sum) {
          stinger_int64_fetch_add(&no, 1);
          if(tree[j] == i) {
            for(int64_t p = 0; p < deg_j; p++) {
              int64_t w = neighbors[my_w + p];
              while(tree[w] != w && w != i && w != j) w = tree[w];
              if(w == ncomp[j] || w == i) {
#if SPAN_TREE_STATS
                stinger_uint64_fetch_add(&deletes_in_tree_caught_climbing, 1);
#endif
                tree[j] = neighbors[my_w + p];
                break;
              }
            }
          }
        }
#if SPAN_TREE_STATS
        else stinger_int64_fetch_add(&deletes_in_tree_caught_bitarrays, 1);
#endif
      }
#endif

#if CHECK_ACCURACY
    connected_components_stinger_edge_parallel_and_tree(nv, ne, S, tempcomp, NULL, temptree);
    OMP("omp parallel for reduction(+:deletes_that_split)")
      for (int64_t k = 0; k < ones; k+=2) {
        if(tempcomp[delA[k]] != tempcomp[delA[k+1]])
          deletes_that_split++;
      }
    printf("unnacounted deletes %ld deletes that sever an edge%ld\n", deletes_in_tree - deletes_in_tree_caught_bitarrays - deletes_in_tree_caught_climbing, deletes_that_split); fflush(stdout);
#endif



    //printf("num_remove_phase = %d\n", num_remove_phase);
    if (num_remove_phase < 0) {
      num_remove_phase = 0;
      num_comp = connected_components_stinger_edge_parallel_and_tree(nv, ne, S, ncomp, NULL, tree);
      printf("num_comp = %ld\n", num_comp);
    }
    else
      {
        /* If we ran connected components above, we don't need to continue.
           Here we transform the insertions from vertices to their respective components. */
        int64_t incr = 0;

        MTA("mta assert nodep")
          OMP("omp parallel for")
          for (int k = 0; k < n; k++) {
            const int64_t i = ACTI2(deloff[k]);
            assert(i >= 0);
            assert(i < nv);
            int k2;
            const int64_t ncompi = ncomp[i];
            willchange[k] = ncompi;

            for (k2 = insoff[k]; k2 < deloff[k+1]; k2++) {
              assert(ACTJ2(k2) >= 0);
              assert(ACTJ2(k2) < nv);
              const int64_t ncompk = ncomp[ACTJ2(k2)];
              if (ncompk != ncompi) {
                int where = stinger_int64_fetch_add(&incr, 1);
                assert(where < 2*batch_size);
                m1[where] = ncompi;
                m2[where] = ncompk;
              }
            }
          }

        num_comp = connected_components_edge(nv, incr, m1, m2, ncomp);

        OMP("omp parallel for")
          for(int k = 0; k < n; k++) {
            const int64_t i = willchange[k];
            if(ncomp[i] != i) {
              int64_t change = ncompsize[i];
              stinger_int64_fetch_add(&ncompsize[ncomp[i]], change);
              ncompsize[i] = 0;
            }
          }
      }      // end "add only" component tracking

    update_time_trace[ntrace] = toc();
    ++ntrace;

  }  /* End each batch */


#if SPAN_TREE_STATS
  PRINT_STAT_INT64 ("total_deletions", deletes_total);
  PRINT_STAT_INT64 ("deletions_in_tree", deletes_in_tree);
  printf(",\n\t\"deletions_in_tree\": %f%%", 100.0f * deletes_in_tree / deletes_total);
  PRINT_STAT_INT64 ("deletions_in_tree_caught_by_bitarrays", deletes_in_tree_caught_bitarrays);
  printf(",\n\t\"deletions_in_tree_caught_by_bitarrays\": %f%%", 100.0f * deletes_in_tree_caught_bitarrays / deletes_in_tree);
  PRINT_STAT_INT64 ("deletions_in_tree_caught_by_climbing", deletes_in_tree_caught_climbing);
  printf(",\n\t\"deletions_in_tree_caught_by_climbing\": %f%%", 100.0f * deletes_in_tree_caught_climbing / deletes_in_tree);
#if CHECK_ACCURACY
  PRINT_STAT_INT64 ("deletes_that_split", deletes_that_split);
#endif
#endif

#if CHECK_ACCURACY
  free(tempcomp);
  free(temptree);
#endif
  free(willchange);
  free(comp_to_merge);
  free(start);
  free(T);
  free(marks);
  free(crossU);
  free(crossV);
  free(neighbors);

  PRINT_STAT_INT64 ("num_components", num_comp);

  num_comp_end = connected_components_stinger (S, nv, ne, ncomp_end, NULL, NULL, NULL, NULL, NULL);
  PRINT_STAT_INT64 ("num_comp_end", num_comp_end);
  if (num_comp != num_comp_end) {
    printf(",\n\tWARNING: final results disagree");
  }

  /* Print the times */
  double total_updates = 0;
  for (int64_t k = 0; k < max_ntrace; k++) {
    total_updates += update_time_trace[k];
  }
  PRINT_STAT_DOUBLE ("total_update_time", total_updates);
  PRINT_STAT_DOUBLE ("updates_per_sec", (nbatch * batch_size) / total_updates);

#if USE_BIT_ARRAY
  for(int64_t k = 0; k < num_bit_arrays; ++k) {
    free(bitarray[k]);
  }
  free(bitarray_off);
#endif

  stinger_free_all (S);
  free(graphmem);
  free(actionmem);
  free(update_time_trace);
  free(ncompsize);
  free(ncomp);
  free(m1);

  STATS_END();
}

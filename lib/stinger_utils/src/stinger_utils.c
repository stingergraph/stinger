/* -*- mode: C; mode: folding; fill-column: 70; -*- */
#include <stdio.h>

#include "stinger_utils.h"
#include "timer.h"

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"

#if defined(_OPENMP)
#include <omp.h>
#endif

/* {{{ Command line parsing */

/**
* @brief Prints command line input information and defaults to the screen.
*
* @param out Where to write, likely stderr.
* @param progname The program binary, likely argv[0].
*/
void
usage (FILE * out, char *progname)
{
  fprintf (out,
           "%s [--batch|-b #] [--num-batches|-n #] [initial-graph.bin [action-stream.bin]]\n"
           "\tDefaults:\n"
           "\t           batch size = %d\n"
           "\t    number of batches = %d\n"
           "\t   initial-graph name = \"%s\"\n"
           "\t   action-stream name = \"%s\"\n",
#if !defined(__MTA__)
           basename (progname),
#else
           progname,
#endif
           BATCH_SIZE_DEFAULT, NBATCH_DEFAULT,
           INITIAL_GRAPH_NAME_DEFAULT, ACTION_STREAM_NAME_DEFAULT);
}

/**
* @brief Parses command line arguments.
*
* Parses the command line input as given by usage().  Batch size, number of
* batches, initial graph filename, and action stream filename are given by
* to the caller if they were specified on the command line.
*
* @param argc The number of arguments
* @param argv[] The array of arguments
* @param initial_graph_name Path/filename of the initial input graph on disk
* @param action_stream_name Path/filename of the action stream on disk
* @param batch_size Number of edge actions to consider in one batch
* @param nbatch Number of batchs to process
*/
void
parse_args (const int argc, char *argv[],
            char **initial_graph_name, char **action_stream_name,
            int64_t * batch_size, int64_t * nbatch)
{
  int k = 1;
  int seen_batch = 0, seen_nbatch = 0;
  if (k >= argc)
    return;
  while (k < argc && argv[k][0] == '-') {
    if (0 == strcmp (argv[k], "--batch") || 0 == strcmp (argv[k], "-b")) {
      if (seen_batch)
        goto err;
      seen_batch = 1;
      ++k;
      if (k >= argc)
        goto err;
      *batch_size = strtol (argv[k], NULL, 10);
      if (batch_size <= 0)
        goto err;
      ++k;
    } else if (0 == strcmp (argv[k], "--num-batches")
             || 0 == strcmp (argv[k], "-n")) {
      if (seen_nbatch)
        goto err;
      seen_nbatch = 1;
      ++k;
      if (k >= argc)
        goto err;
      *nbatch = strtol (argv[k], NULL, 10);
      if (nbatch < 0)
        goto err;
      ++k;
    } else if (0 == strcmp (argv[k], "--help")
             || 0 == strcmp (argv[k], "-h") || 0 == strcmp (argv[k], "-?")) {
      usage (stdout, argv[0]);
      exit (EXIT_SUCCESS);
      return;
    } else if (0 == strcmp (argv[k], "--")) {
      ++k;
      break;
    }
  }
  if (k < argc)
    *initial_graph_name = argv[k++];
  if (k < argc)
    *action_stream_name = argv[k++];
  return;
 err:
  usage (stderr, argv[0]);
  exit (EXIT_FAILURE);
  return;
}

/* }}} */

/**
* @brief Tests if the string is less than 255 characters and contains only _, a-z, A-Z, 0-9
*
* @param name The string.
* @param length The length of the string.
*
* @return 1 if the string is "simple", 0 otherwise
*/
int
is_simple_name(const char * name, int64_t length)
{
  if(length > 254)
    return 0;

  for(int64_t i = 0; i < length; i++) {
    char c = name[i];
    if(!(
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '_'))
      return 0;
  }

  return 1;
}


/**
* @brief A basic counting sort
*
* @param array Input array of numbers to be sorted
* @param num Number of items to be sorted
* @param size Size of each item in array[]
*/
void
counting_sort (int64_t * array, size_t num, size_t size)
{
  int64_t i, min, max;

  min = max = array[0];

  for (i = size; i < num * size; i += size) {
    if (array[i] < min)
      min = array[i];
    else if (array[i] > max)
      max = array[i];
  }

  int range = max - min + 1;
  int64_t *count = xmalloc (range * sizeof (int64_t));

  for (i = 0; i < range; i++)
    count[i] = 0;

  for (i = 0; i < num * size; i += size)
    count[array[i] - min]++;

  int64_t j, z = 0;
  for (i = min; i <= max; i++)
    for (j = 0; j < count[i - min]; j++)
      array[z++] = i;

  free (count);
}

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

/**
* @brief Prints basic statistics about the graph loaded from disk
*
* @param nv Number of vertices
* @param ne Number of edges
* @param batch_size Batch size of the action stream (specified on the command line)
* @param nbatch Number of batches (specified on the command line)
* @param naction Number of edge actions in the action stream
*/
void
print_initial_graph_stats (int64_t nv, int64_t ne, int64_t batch_size,
                           int64_t nbatch, int64_t naction)
{
  printf (",\n\t\"nv\": %ld", (long int) nv);
  printf (",\n\t\"ne\": %ld", (long int) ne);
  printf (",\n\t\"batchsize\": %ld", (long int) batch_size);
  printf (",\n\t\"nbatch\": %ld", (long int) nbatch);
  printf (",\n\t\"naction\": %ld", (long int) naction);
}

/**
* @brief Convert a plain edge list to compressed sparse row (CSR) format. If one 
* or both timestamp input arrays are NULL, they will be ignored.
*
* @param nv Number of vertices
* @param ne Number of edges
* @param sv1 Input array of source vertices
* @param ev1 Input array of destination vertices
* @param timeRecent Array of timestamps or NULL
* @param timeFirst Array of timestamps or NULL
* @param w1 Input array of edge weights
* @param ev2 Output array of destination vertices
* @param w2 Output array of edge weights
* @param offset Output array of vertex offsets into ev2[], w2[], t2[], and t1[]
* @param t2 Output array of timestamps or NULL
* @param t1 Output array of timestamps or NULL
*/
#if 0
// BROKEN now that direction is required on each edge
void
edge_list_to_csr (int64_t nv, int64_t ne,
                  int64_t * sv1, int64_t * ev1, int64_t * w1, int64_t * timeRecent, int64_t * timeFirst,
                  int64_t * ev2, int64_t * w2, int64_t * offset, int64_t * t2, int64_t * t1)
{
  int64_t i;

  OMP ("omp parallel for")
    for (i = 0; i < nv + 2; i++)
      offset[i] = 0;

  offset += 2;

  /* Histogram source vertices for outer sort */
  OMP ("omp parallel for")
    MTA ("mta assert no alias *offset *sv1")
    for (i = 0; i < ne; i++)
      stinger_int64_fetch_add (&offset[sv1[i]], 1);

  /* Compute offset index of each bucket */
  for (i = 1; i < nv; i++)
    offset[i] += offset[i - 1];

  offset--;

  if(timeRecent && timeFirst) {
    OMP ("omp parallel for") MTA ("mta assert nodep")
    for (i = 0; i < ne; i++) {
      int64_t index = stinger_int64_fetch_add (&offset[sv1[i]], 1);
      ev2[index] = ev1[i];
      w2[index] = w1[i];
      t1[index] = timeFirst[i];
      t2[index] = timeRecent[i];
    }
  } else {
    OMP ("omp parallel for") MTA ("mta assert nodep")
    for (i = 0; i < ne; i++) {
      int64_t index = stinger_int64_fetch_add (&offset[sv1[i]], 1);
      ev2[index] = ev1[i];
      w2[index] = w1[i];
    }
  }
}
#endif

/**
* @brief Take a plain edge list and convert it into a STINGER. If only 
* recent timestamps or modified timestamps are given, they will be used 
* for both.  If neither are given, the default is used.
*
* @param nv Number of vertices
* @param ne Number of edges
* @param sv Array of source vertices
* @param ev Array of destination vertices
* @param w Array of edge weights
* @param timeRecent Array of timestamps or NULL
* @param timeFirst Array of timestamps or NULL
* @param timestamp Default timestamp (if recent and modified not given)
* 
* @return A STINGER data structure
*/
struct stinger *
edge_list_to_stinger (int64_t nv, int64_t ne,
                      int64_t * sv, int64_t * ev, int64_t * w, 
		      int64_t * timeRecent, int64_t * timeFirst, 
		      int64_t timestamp)
{
  /* if only one is given, use for both */
  if(timeRecent == NULL) {
    timeRecent = timeFirst;
  } else if (timeFirst == NULL) {
    timeFirst = timeRecent;
  }

  struct stinger *S = stinger_new ();

  OMP("omp parallel for")
  for (int64_t i = 0; i < ne; i++) {
    stinger_insert_edge (S, 0, sv[i], ev[i], w[i], (timeFirst)?timeFirst:timestamp);
    if (timeRecent && timeRecent != timeFirst) {
      stinger_edge_touch (S, sv[i], ev[i], 0, timeRecent);
    }
  }

  return S;
}

/**
* @brief For a given vertex and edge type in STINGER, sort the adjacency list
*
* This function sorts the linked block data structure inside STINGER for a
* particular vertex ID and edge type.  Since STINGER is assumed to be changing,
* we cannot guarantee that the adjacency list will remain sorted.  We provide
* this function such that some algorithms may see a small speed-up if sorted or
* partially sorted.
*
* This function is currently EXPERIMENTAL.  There are known bugs.  Please report
* bugs to the development team.
*
* @param S The STINGER data structure
* @param srcvtx Vertex ID of the adjacency list to sort
* @param type Edge type of the adjacency list to sort
*/
void
stinger_sort_edge_list (const struct stinger *S, const int64_t srcvtx,
                        const int64_t type)
{
  MAP_STING(S);
  struct stinger_eb * ebpool_priv = ebpool->ebpool;

  int64_t sorted = 0;
  struct stinger_eb *cur_eb;
  struct stinger_eb *next_eb;

  struct stinger_eb *start = ebpool_priv + stinger_vertex_edges_get(vertices, srcvtx);
  while (start != ebpool_priv && start->etype != type) {
    start = ebpool_priv + start->next;
  }

  while (!sorted) {
    cur_eb = start;

    sorted = 1;
    while (cur_eb != ebpool_priv && cur_eb->etype == type) {
      next_eb = ebpool_priv + cur_eb->next;

      for (uint64_t i = 1; i < STINGER_EDGEBLOCKSIZE; i += 2) {
        if (i < STINGER_EDGEBLOCKSIZE - 1) {
          if (stinger_eb_adjvtx(cur_eb,i) > stinger_eb_adjvtx(cur_eb,i+1)) {
            struct stinger_edge tmp = cur_eb->edges[i + 1];
            cur_eb->edges[i + 1] = cur_eb->edges[i];
            cur_eb->edges[i] = tmp;
            sorted = 0;
          }
        } else {
          if (cur_eb->next && ebpool_priv[cur_eb->next].etype == type
              && stinger_eb_adjvtx(cur_eb,i) > stinger_eb_adjvtx(next_eb,0)) {
            struct stinger_edge tmp = ebpool_priv[cur_eb->next].edges[0];
            ebpool_priv[cur_eb->next].edges[0] = cur_eb->edges[i];
            cur_eb->edges[i] = tmp;
            sorted = 0;
          }
        }
      }
      cur_eb = ebpool_priv + cur_eb->next;
    }

    cur_eb = start;

    while (cur_eb != ebpool_priv && cur_eb->etype == type) {
      next_eb = ebpool_priv + cur_eb->next;

      for (uint64_t i = 0; i < STINGER_EDGEBLOCKSIZE; i += 2) {
        if (i < STINGER_EDGEBLOCKSIZE - 1) {
          if (stinger_eb_adjvtx(cur_eb,i) > stinger_eb_adjvtx(cur_eb,i+1)) {
            struct stinger_edge tmp = cur_eb->edges[i + 1];
            cur_eb->edges[i + 1] = cur_eb->edges[i];
            cur_eb->edges[i] = tmp;
            sorted = 0;
          }
        } else {
          if (cur_eb->next && ebpool_priv[cur_eb->next].etype == type
              && stinger_eb_adjvtx(cur_eb,i) > stinger_eb_adjvtx(next_eb,0)) {
            struct stinger_edge tmp = ebpool_priv[cur_eb->next].edges[0];
            ebpool_priv[cur_eb->next].edges[0] = cur_eb->edges[i];
            cur_eb->edges[i] = tmp;
            sorted = 0;
          }
        }
      }
      cur_eb = ebpool_priv + cur_eb->next;
    }
  }

  cur_eb = start;
  while (cur_eb != ebpool_priv && cur_eb->etype == type) {
    int64_t curLargeTS = INT64_MIN;
    int64_t curSmallTS = INT64_MAX;
    int64_t curNumEdges = 0;
    int64_t curHigh = 0;

    for (uint64_t i = 0; i < STINGER_EDGEBLOCKSIZE; i++) {
      if (!stinger_eb_is_blank (cur_eb, i)) {
        if (stinger_eb_adjvtx(cur_eb,i) == 0
            && stinger_eb_weight(cur_eb,i) == 0
            && stinger_eb_first_ts(cur_eb,i) == 0
            && stinger_eb_ts(cur_eb,i) == 0) {
          cur_eb->edges[i].neighbor = -1;
        } else {
          curNumEdges++;
          if (i > curHigh)
            curHigh = i;
          if (cur_eb->edges[i].timeFirst < curSmallTS)
            curSmallTS = stinger_eb_first_ts(cur_eb,i);
          if (cur_eb->edges[i].timeRecent > curLargeTS)
            curLargeTS = stinger_eb_ts(cur_eb,i);
        }
      }
    }

    cur_eb->high = curHigh + 1;
    cur_eb->numEdges = curNumEdges;
    cur_eb->largeStamp = curLargeTS;
    cur_eb->smallStamp = curSmallTS;
    cur_eb = ebpool_priv + cur_eb->next;
  }
}


/**
* @brief A simple bucket sort for an array of tuples
*
* This function sorts an array of tuples, first by the leading element, then by
* the trailing element.  It accomplishes this by bucketing by the first element,
* then sorting by the second element within the bucket.
*
* On the Cray XMT, this sort performed very well up to 32 processors.  Hot-spotting
* and load imbalance make it infeasible for large processor counts.
*
* @param array Input array of tuples to sort
* @param num Number of tuples to sort
*/
MTA("mta inline")
void
bucket_sort_pairs (int64_t *array, size_t num)
{
  int64_t i;
  int64_t max, min;
  int64_t *tmp = xmalloc (2 * num * sizeof(int64_t));

  max = min = array[0];

  MTA("mta assert nodep")
  for (i = 2; i < num<<1; i+=2)
    if (max < array[i])
      max = array[i];

  MTA("mta assert nodep")
  for (i = 2; i < num<<1; i+=2)
    if (min > array[i])
      min = array[i];

  int64_t range = max - min + 1;
  int64_t offset = 0 - min;

  int64_t *start = xmalloc ( (range+2) * sizeof(int64_t) );

  OMP("omp parallel for")
  for (i = 0; i < range + 2; i++)
    start[i] = 0;

  start += 2;

  /* Histogram key values */
  MTA("mta assert no alias *start *array")
  for (i = 0; i < num<<1; i+=2)
    start[array[i]+offset] += 2;

  /* Compute start index of each bucket */
  for (i = 0; i < range; i++)
    start[i] += start[i-1];

  start --;

  /* Move edges into its bucket's segment */
  OMP("omp parallel for")
  MTA("mta assert nodep")
  for (i = 0; i < num<<1; i+=2) {
    int64_t index = stinger_int64_fetch_add(start+array[i]+offset, 2);
    tmp[index] = array[i];
    tmp[index+1] = array[i+1];
  } 

  /* Copy back from tmp to the original array */
  OMP("omp parallel for")
  for (i = 0; i < num<<1; i++) {
    array[i] = tmp[i];
  }

  start--;

  OMP("omp parallel for")
  MTA("mta assert parallel")
  for (i = 0; i < range; i++) {
    int64_t degree = start[i+1] - start[i];
    if (degree >= 4)
      qsort(&array[start[i]], degree>>1, 2*sizeof(int64_t), i64_pair_cmp);
  }

  free(start);
  free(tmp);
}

/**
* @brief A radix sort for edge tuples
*
* This function replaces the bucket_sort_pairs() in STINGER.
*
* @param x Input array of tuples sort
* @param length Number of tuples to sort
* @param numBits Number of bits to use for the radix
*/
void radix_sort_pairs (int64_t *x, int64_t length, int64_t numBits)
{
  int64_t max, min;
  int64_t numBuckets = 1 << numBits;
  int64_t bitMask = numBuckets - 1;
  int64_t * buckets = xmalloc ( (numBuckets + 2) * sizeof(int64_t));
  int64_t * copy1 = xmalloc ( length * sizeof(int64_t));
  int64_t * copy2 = xmalloc ( length * sizeof(int64_t));
  int64_t * tmp;

  assert(length % 2 == 0);

  max = x[1];
  min = x[1];
  for (int64_t i = 0; i < length; i+=2) {
    copy1[i] = x[i];
    copy1[i+1] = x[i+1];
    if (max < x[i+1]) {
      max = x[i+1];
    }
    if (min > x[i+1]) {
      min = x[i+1];
    }
  }
  min = -min;
  max += min;

  for (int64_t i = 0; i < length; i+=2)
  {
    copy1[i+1] += min;
  }

  int64_t denShift = 0;
  for (int64_t i = 0; max != 0; max = max / numBuckets, i++)
  {
    for (int64_t j = 0; j < numBuckets + 2; j++)
    {
      buckets[j] = 0;
    }

    buckets += 2;

    for (int64_t j = 0; j < length; j+=2)
    {
      int64_t myBucket = (int64_t) ((copy1[j+1]) >> denShift) & bitMask;
      assert (myBucket >= 0);
      assert (myBucket < numBuckets);
      stinger_int64_fetch_add (&buckets[myBucket], 2);
    }

    for (int64_t j = 1; j < numBuckets; j++)
    {
      buckets[j] += buckets[j-1];
    }

    buckets--;

    for (int64_t j = 0; j < length; j+=2)
    {
      int64_t myBucket = (int64_t) ((copy1[j+1]) >> denShift) & bitMask;
      int64_t index = stinger_int64_fetch_add (&buckets[myBucket], 2);
      copy2[index] = copy1[j];
      copy2[index+1] = copy1[j+1];
    }

    buckets--;
    denShift += numBits;

    tmp = copy1;
    copy1 = copy2;
    copy2 = tmp;
  }

  max = copy1[0];
  for (int64_t i = 0; i < length; i+=2) {
    if (max < copy1[i]) {
      max = copy1[i];
    }
  }

  denShift = 0;
  for (int64_t i = 0; max != 0; max = max / numBuckets, i++)
  {
    for (int64_t j = 0; j < numBuckets + 2; j++)
    {
      buckets[j] = 0;
    }

    buckets += 2;

    for (int64_t j = 0; j < length; j+=2)
    {
      int64_t myBucket = (int64_t) (copy1[j] >> denShift) & bitMask;
      stinger_int64_fetch_add (&buckets[myBucket], 2);
    }

    for (int64_t j = 1; j < numBuckets; j++)
    {
      buckets[j] += buckets[j-1];
    }

    buckets--;

    for (int64_t j = 0; j < length; j+=2)
    {
      int64_t myBucket = (int64_t) (copy1[j] >> denShift) & bitMask;
      int64_t index = stinger_int64_fetch_add (&buckets[myBucket], 2);
      copy2[index] = copy1[j];
      copy2[index+1] = copy1[j+1];
    }

    buckets--;
    denShift += numBits;

    tmp = copy1;
    copy1 = copy2;
    copy2 = tmp;
  }


  for (int64_t i = 0; i < length; i+=2)
  {
    copy1[i+1] -= min;
  }

  for (int64_t i = 0; i < length; i++)
  {
    x[i] = copy1[i];
  }

  free(copy2);
  free(copy1);
  free(buckets);

}

/**
 * @brief Simple comparator for int64_t.
 *
 * @param a Pointer to first integer.
 * @param b Pointer to second integer.
 * @return > 0 if a is greater, < 0 if b is greater, 0 if equal.
 */
int
i64_cmp (const void *a, const void *b)
{
  const int64_t va = *(const int64_t *) a;
  const int64_t vb = *(const int64_t *) b;
  return va - vb;
}

/**
 * @brief Simple comparator for pairs of int64_t.
 *
 * integers in a pair must be stored contiguously.
 *
 * @param a Pointer to first integer pair.
 * @param b Pointer to second integer pair.
 * @return > 0 if a is greater, < 0 if b is greater, 0 if equal.
 */
MTA ("mta inline")
int i2cmp (const void *va, const void *vb)
{
  const int64_t *a = va;
  const int64_t *b = vb;
  if (a[0] < b[0])
    return -1;
  if (a[0] > b[0])
    return 1;
  if (a[1] < b[1])
    return -1;
  if (a[1] > b[1])
    return 1;
  return 0;
}

/**
 * @brief Find an element in a sorted array of int64_t.
 *
 * @param tofind The value.
 * @param N Array length.
 * @param ary Array pointer.
 * @return Index in the array or -1 if not found.
 */
MTA ("mta inline") MTA ("mta expect parallel context")
int64_t
find_in_sorted (const int64_t tofind,
                const int64_t N, const int64_t * restrict ary)
{
  int64_t out = -1;
  if (N <= 0)
    return -1;

  int64_t top = N - 1, bot = 0;

  if (tofind == ary[bot])
    return bot;
  if (tofind == ary[top])
    return top;
  while (top - bot + 1 > 4) {
    int64_t mid = bot + ((top - bot) / 2);
    if (tofind == ary[mid])
      return mid;
    if (tofind < ary[mid])
      top = mid;
    else
      bot = mid;
  }
  for (; bot <= top; ++bot)
    if (tofind == ary[bot])
      return bot;
  return -1;
}

/** @brief Inclusive prefix sum utility function.
 *
 * This sum is inclusive meaning that the first element of the output will be
 * equal to the first element of the input (not necessarily 0).
 *
 * If compiled with OpenMP, assumes that you are in parallel section and are 
 * calling this with all threads (although it will also work outside of 
 * a parallel OpenMP context - just DO NOT call this in OpenMP single or master
 * inside of a parallel region).
 *
 * If compiled without OpenMP this is implemented as a simple serial prefix sum  
 * that should be auto-parallelized by the Cray MTA/XMT compiler.
 *
 * @param n The input array size.
 * @param ary The input array pointer.
 * @return The final element of the sum.
 */
int64_t
prefix_sum (const int64_t n, int64_t *ary); /* re-declared here for doxygen */
#if defined(_OPENMP)
int64_t
prefix_sum (const int64_t n, int64_t *ary)
{
  static int64_t *buf;

  int nt, tid;
  int64_t slice_begin, slice_end, t1, k, tmp;

  nt = omp_get_num_threads ();
  tid = omp_get_thread_num ();

  OMP("omp master")
    buf = alloca (nt * sizeof (*buf));
  OMP("omp flush (buf)");
  OMP("omp barrier");

  slice_begin = (tid * n) / nt;
  slice_end = ((tid + 1) * n) / nt; 

  /* compute sums of slices */
  tmp = 0;
  for (k = slice_begin; k < slice_end; ++k)
    tmp += ary[k];
  buf[tid] = tmp;

  /* prefix sum slice sums */
  OMP("omp barrier");
  OMP("omp single")
    for (k = 1; k < nt; ++k)
      buf[k] += buf[k-1];
  OMP("omp barrier");

  /* get slice sum */
  if (tid)
    t1 = buf[tid-1];
  else
    t1 = 0;

  /* prefix sum our slice including previous slice sums */
  ary[slice_begin] += t1;
  for (k = slice_begin + 1; k < slice_end; ++k) {
    ary[k] += ary[k-1];
  }
  OMP("omp barrier");

  return ary[n-1];
}

#else
MTA("mta inline")
int64_t
prefix_sum (const int64_t n, int64_t *ary)
{
  for (int64_t k = 1; k < n; ++k) {
    ary[k] += ary[k-1];
  }
  return ary[n-1];
}
#endif

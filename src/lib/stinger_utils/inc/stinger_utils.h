#if !defined(STINGER_UTILS_H_)
#define STINGER_UTILS_H_

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

#include "stinger_core/stinger.h"

#define INITIAL_GRAPH_NAME_DEFAULT "initial-graph.bin"
#define ACTION_STREAM_NAME_DEFAULT "action-stream.bin"
#define BATCH_SIZE_DEFAULT 1
#define NBATCH_DEFAULT 100

#define STATS_INIT()							\
  do {									\
    init_timer ();							\
    printf ("{\n");							\
    printf ("\t\"timer_res\": %20.15e,", timer_getres ());		\
  } while (0)

#define STATS_END() do { printf ("\n}\n"); } while (0)

#define BATCH_SIZE_CHECK()					\
  do {								\
    if (naction < nbatch * batch_size) {			\
      fprintf (stderr, "WARNING: not enough actions\n");	\
      nbatch = (naction + batch_size - 1) / batch_size;		\
    }								\
  } while (0)

#define PRINT_STAT_INT64(NAME_, VALUE_)		\
  do {						\
    printf(",\n\t\"%s\": %ld", NAME_, VALUE_); \
  } while (0)

#define PRINT_STAT_INT64_ARRAY_AS_PAIR(NAME_, ARY_, LEN_)		\
  do {						\
    printf(",\n\t\"%s\": [", NAME_);    \
    for(uint64_t i = 0; i < LEN_ - 1; i++) { \
      printf("\n\t\t[ %ld, %ld],", i, ARY_ [i]); \
    } \
    printf("\n\t\t[ %ld, %ld]", LEN_ -1, ARY_ [LEN_ -1]); \
    printf("\n\t]"); \
  } while (0)

#define PRINT_STAT_DOUBLE(NAME_, VALUE_)	\
  do {						\
    printf(",\n\t\"%s\": %20.15e", NAME_, VALUE_); \
  } while (0)

#define PRINT_STAT_HEX64(NAME_, VALUE_)	\
  do {						\
    printf(",\n\t\"%s\":\"0x%lx\"", NAME_, VALUE_); \
  } while (0)

void usage (FILE * out, char *progname);

void parse_args (const int argc, char *argv[],
		 char **initial_graph_name, char **action_stream_name,
		 int64_t * batch_size, int64_t * nbatch);

int
is_simple_name(const char * name, int64_t length);

/* Freeing mem_handle frees all associated memory. */
void snarf_graph (const char *initial_graph_name,
		  int64_t * nv_out, int64_t * ne_out,
		  int64_t ** off_out, int64_t ** ind_out,
		  int64_t ** weight_out, int64_t ** mem_handle);

void snarf_actions (const char *action_stream_name,
		    int64_t * naction_out, int64_t ** action_out,
		    int64_t ** mem_handle);

void load_graph_and_action_stream (const char *initial_graph_name,
				   int64_t * nv, int64_t * ne,
				   int64_t ** off, int64_t ** ind,
				   int64_t ** weight, int64_t ** graphmem,
				   const char *action_stream_name,
				   int64_t * naction, int64_t ** action,
				   int64_t ** actionmem);

void stinger_to_sorted_csr (const struct stinger *G, const int64_t nv,
                         int64_t ** off_out, int64_t ** ind_out, int64_t ** weight_out, 
			 int64_t ** timefirst_out, int64_t ** timerecent_out, int64_t ** type_out);

void stinger_to_unsorted_csr (const struct stinger *G, const int64_t nv,
                         int64_t ** off_out, int64_t ** ind_out, int64_t ** weight_out, 
			 int64_t ** timefirst_out, int64_t ** timerecent_out, int64_t ** type_out);

void counting_sort (int64_t * array, size_t num, size_t size);

int64_t find_in_sorted (const int64_t tofind, const int64_t N, const int64_t * ary);

void print_initial_graph_stats (int64_t nv, int64_t ne, int64_t batch_size,
				int64_t nbatch, int64_t naction);

void edge_list_to_csr (int64_t nv, int64_t ne,
                  int64_t * sv1, int64_t * ev1, int64_t * w1, int64_t * timeRecent, int64_t * timeFirst,
                  int64_t * ev2, int64_t * w2, int64_t * offset, int64_t * t2, int64_t * t1);

struct stinger *edge_list_to_stinger (int64_t nv, int64_t ne,
                      int64_t * sv, int64_t * ev, int64_t * w, 
		      int64_t * timeRecent, int64_t * timeFirst, 
		      int64_t timestamp);

void stinger_sort_edge_list (const struct stinger *S, const int64_t srcvtx, const int64_t type);

void bucket_sort_pairs (int64_t *array, size_t num);

void radix_sort_pairs (int64_t *x, int64_t length, int64_t numBits);

int64_t bs64 (int64_t xin);

void bs64_n (size_t n, int64_t * d);

int i64_cmp (const void *a, const void *b);

int i2cmp (const void *va, const void *vb);

int64_t find_in_sorted (const int64_t tofind,
                const int64_t N, const int64_t * ary);

int64_t prefix_sum (const int64_t n, int64_t *ary);

void
stinger_extract_bfs (/*const*/ struct stinger *S,
                     const int64_t nsrc, const int64_t * srclist_in,
                     const int64_t * label_in,
                     const int64_t max_nv_out,
                     const int64_t max_nlevels,
                     int64_t * nv_out,
                     int64_t * vlist_out /* size >=max_nv_out */,
                     int64_t * mark_out /* size nv, zeros */);

void
stinger_extract_mod (/*const*/ struct stinger *S,
                     const int64_t nsrc, const int64_t * srclist_in,
                     const int64_t * label_in,
                     const int64_t max_nv_out,
                     int64_t * nv_out,
                     int64_t * vlist_out /* size >=max_nv_out */,
                     int64_t * mark_out /* size nv, zeros */);


#ifdef __cplusplus
}
#undef restrict
#endif

#endif /* STINGER_UTILS_H_ */

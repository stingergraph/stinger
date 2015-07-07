#include    "stinger_core/stinger_atomics.h"
#include    "stinger_utils/timer.h"

#include    "random.h"
#include    "alloca.h"

#include  <ctype.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <stdint.h>
#include  <getopt.h>
#if defined(_OPENMP)
#include  <omp.h>
#endif

#define NULLCHECK(X) do { if(NULL == (X)) {printf("%s %d NULL: %s", __func__, __LINE__, #X); abort(); } } while(0);
#define ZEROCHECK(X) do { if(0 == (X)) {printf("%s %d ZERO: %s", __func__, __LINE__, #X); abort(); } } while(0);

#if defined(_OPENMP)
#define OMP(x) _Pragma(x)
#else
#define OMP(x)
#endif

int
i64_cmp(const void * a, const void * b) {
  return (*((int64_t *) a)) - (*((int64_t *) b));
}

int64_t
prefix_sum (const int64_t n, int64_t *ary)
{
  static int64_t *buf;

  int nt, tid;
  int64_t slice_begin, slice_end, t1, k, tmp;

#if defined(_OPENMP)
  nt = omp_get_num_threads ();
  tid = omp_get_thread_num ();
#else
  nt = 1;
  tid = 0;
#endif

  OMP("omp master")
    buf = alloca (nt * sizeof (*buf));
  OMP("omp flush (buf)")
  OMP("omp barrier")

  slice_begin = (tid * n) / nt;
  slice_end = ((tid + 1) * n) / nt; 

  /* compute sums of slices */
  tmp = 0;
  for (k = slice_begin; k < slice_end; ++k)
    tmp += ary[k];
  buf[tid] = tmp;

  /* prefix sum slice sums */
  OMP("omp barrier")
  OMP("omp single")
    for (k = 1; k < nt; ++k)
      buf[k] += buf[k-1];
  OMP("omp barrier")

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
  OMP("omp barrier")

  return ary[n-1];
}

static void
rmat_edge (int64_t * iout, int64_t * jout,
           int SCALE, double A, double B, double C, double D, dxor128_env_t * env)
{
  int64_t i = 0, j = 0;
  int64_t bit = ((int64_t) 1) << (SCALE - 1);

  while (1) {
    const double r = dxor128(env);
    if (r > A) {                /* outside quadrant 1 */
      if (r <= A + B)           /* in quadrant 2 */
        j |= bit;
      else if (r <= A + B + C)  /* in quadrant 3 */
        i |= bit;
      else {                    /* in quadrant 4 */
        j |= bit;
        i |= bit;
      }
    }
    if (1 == bit)
      break;

    /*
      Assuming R is in (0, 1), 0.95 + 0.1 * R is in (0.95, 1.05).
      So the new probabilities are *not* the old +/- 10% but
      instead the old +/- 5%.
    */
    A *= (9.5 + dxor128(env)) / 10;
    B *= (9.5 + dxor128(env)) / 10;
    C *= (9.5 + dxor128(env)) / 10;
    D *= (9.5 + dxor128(env)) / 10;
    /* Used 5 random numbers. */

    {
      const double norm = 1.0 / (A + B + C + D);
      A *= norm;
      B *= norm;
      C *= norm;
    }
    /* So long as +/- are monotonic, ensure a+b+c+d <= 1.0 */
    D = 1.0 - (A + B + C);

    bit >>= 1;
  }
  /* Iterates SCALE times. */
  *iout = i;
  *jout = j;
}



int main(int argc, char *argv[]) {
  uint64_t SCALE = 20;
  uint64_t EDGEFACTOR = 8;
  uint64_t NUM_ACTIONS = 100000;

  uint64_t nv, ne;
  uint64_t * src, * dst;
  uint64_t * off, * of2, * ind, * in2, * wgt, * wg2;

  int64_t * actions, a_count = 0;

  double A = 0.55;
  double B = 0.1;
  double C = 0.1;
  double D = 0.25;
  double P_delete = 0.0625;

  char * graph_file = NULL;
  char * actions_file = NULL;

  const uint64_t endian_check = 0x1234ABCDul;

  FILE * fp_graph, * fp_actions;

  int c;
  while(-1 != (c = getopt(argc, argv, "s:S:e:E:n:N:g:G:a:A:"))) {
    switch(c) {
      case 's':
      case 'S':
	SCALE = atol(optarg);
      break;
      case 'e':
      case 'E':
	EDGEFACTOR = atol(optarg);
      break;
      case 'n':
      case 'N':
	NUM_ACTIONS = atol(optarg);
      break;
      case 'a':
      case 'A':
	actions_file = optarg;
      break;
      case 'g':
      case 'G':
	graph_file = optarg;
      break;
      case '?':
	printf(
	  "Options: \n"
	  "   -s SCALE\n"
	  "   -e EDGEFACTOR\n" 
	  "   -n NUM_ACTIONS\n" 
	  "   -g GRAPH_NAME\n" 
	  "   -a ACTIONS_NAME\n");
	exit(-1);
      break;
    }
  }

  nv = 1ULL << SCALE;
  ne = nv * EDGEFACTOR;

  if(!graph_file) {
    graph_file = malloc(sizeof(char) * 256);
    sprintf(graph_file, "g.%ld.%ld.bin", SCALE, EDGEFACTOR);
  }

  if(!actions_file) {
    actions_file = malloc(sizeof(char) * 256);
    sprintf(actions_file, "a.%ld.%ld.%ld.bin", SCALE, EDGEFACTOR, NUM_ACTIONS);
  }

  printf("%18s : %ld,\n", "SCALE", SCALE);
  printf("%18s : %ld,\n", "EDGEFACTOR", EDGEFACTOR);
  printf("%18s : %ld,\n", "NUM_ACTIONS", NUM_ACTIONS);
  printf("%18s : %lf,\n", "A", A);
  printf("%18s : %lf,\n", "B", B);
  printf("%18s : %lf,\n", "C", C);
  printf("%18s : %lf,\n", "D", D);
  printf("%18s : %lf,\n", "P_delete", P_delete);
  printf("%18s : %ld,\n", "NV", nv);
  printf("%18s : %ld,\n", "NE", ne);
  printf("%18s : %s,\n", "graph_file", graph_file);
  printf("%18s : %s,\n", "actions_file", actions_file);

  NULLCHECK(src = malloc(sizeof(uint64_t) * ne * 2));
  NULLCHECK(dst = malloc(sizeof(uint64_t) * ne * 2));

  /* Generate edges */
  tic();
  OMP("omp parallel")
  {
    dxor128_env_t env;
    #if defined(_OPENMP)
      dxor128_seed(&env, omp_get_thread_num());
    #else
      dxor128_seed(&env, 0);
    #endif

    OMP("omp for")
    for(uint64_t e = 0; e < ne; e++) {
      rmat_edge (&(src[e]), &(dst[e]), SCALE, A, B, C, D, &env);

      /* add reverse edge */
      src[e + ne] = dst[e];
      dst[e + ne] = src[e];
    }
  }

  /* for reverse edges on the end */
  ne *= 2;

  printf("%18s : %lf,\n", "Generation", toc());

  /* Offset calc - ignore self loops */
  NULLCHECK(off = calloc(nv+2, sizeof(uint64_t)));
  NULLCHECK(of2 = calloc(nv+2, sizeof(uint64_t)));
  NULLCHECK(ind = calloc(ne, sizeof(uint64_t)));
  NULLCHECK(in2 = calloc(ne, sizeof(uint64_t)));
  NULLCHECK(wgt = calloc(ne, sizeof(uint64_t)));
  NULLCHECK(wg2 = calloc(ne, sizeof(uint64_t)));

  off += 2;

  tic();

  OMP("omp parallel for")
  for(uint64_t e = 0; e < ne; e++) {
    if(src[e] != dst[e]) {
      stinger_int64_fetch_add(&(off[src[e]]), 1);
    }
  }

  prefix_sum(nv, off);
  off--;
  printf("%18s : %lf,\n", "Offset Calc", toc());

  /* Ind copy - ignore self loops */
  tic();

  OMP("omp parallel for")
  for(uint64_t e = 0; e < ne; e++) {
    if(src[e] != dst[e]) {
      uint64_t place = stinger_int64_fetch_add(&(off[src[e]]), 1);
      ind[place] = dst[e];
    }
  }

  off--;
  printf("%18s : %lf,\n", "Ind Copy", toc());

  /* Sort adjacencies, accumulate duplicates */
  tic();
  of2++;

  OMP("omp parallel for")
  for(uint64_t v = 0; v < nv; v++) {
    qsort(ind + off[v], off[v+1] - off[v], sizeof(uint64_t), i64_cmp); 
    int64_t cur = off[v];
    int64_t last = off[v];

    while(cur < off[v+1]) {
      int64_t weight = 1;
      while(ind[cur] == ind[cur+weight] && cur + weight < off[v+1]) {
	weight++;
      }
      ind[last] = ind[cur];
      wgt[last] = weight;
      cur += weight;
      last++;
    }
    of2[v] = last - off[v];
  }

  prefix_sum(nv, of2);
  of2--;
  printf("%18s : %lf,\n", "Sort, Sum Weights", toc());

  /* Copy unique edges */
  tic();

  OMP("omp parallel for")
  for(uint64_t v = 0; v < nv; v++) {
    int64_t cur = off[v];
    int64_t cr2 = of2[v];
    while(cur < off[v+1] && wgt[cur]) {
      in2[cr2] = ind[cur];
      wg2[cr2] = wgt[cur];
      cr2++; cur++;
    }
  }

  printf("%18s : %lf,\n", "Copy unique", toc());

  ne = of2[nv];

  printf("%18s : %ld,\n", "unique NE", ne);

  /* Graph file write */
  tic();
  NULLCHECK(fp_graph = fopen(graph_file, "w"));

  ZEROCHECK(fwrite(&endian_check, sizeof(uint64_t), 1, fp_graph));
  ZEROCHECK(fwrite(&nv, sizeof(uint64_t), 1, fp_graph));
  ZEROCHECK(fwrite(&ne, sizeof(uint64_t), 1, fp_graph));
  ZEROCHECK(fwrite(of2, sizeof(uint64_t), nv+1, fp_graph));
  ZEROCHECK(fwrite(in2, sizeof(uint64_t), ne, fp_graph));
  ZEROCHECK(fwrite(wg2, sizeof(uint64_t), ne, fp_graph));

  fclose(fp_graph);
  free(off); free(ind); free(wgt);
  free(of2); free(in2); free(wg2);

  printf("%18s : %lf,\n", "Graph file write", toc());

  /* Begin actions */
  tic();

  NULLCHECK(actions = malloc(2 * NUM_ACTIONS * sizeof(int64_t)));

  OMP("omp parallel")
  {
    dxor128_env_t env;

    /* seed with + nv to get different seeds than above */
    #if defined(_OPENMP)
      dxor128_seed(&env, omp_get_thread_num()+nv);
    #else
      dxor128_seed(&env, 0);
    #endif

    OMP("omp for")
    for(uint64_t a = 0; a < NUM_ACTIONS; a++) {
      uint64_t place = stinger_int64_fetch_add(&a_count, 1);
      if(dxor128(&env) > P_delete) {
	rmat_edge (&(actions[place*2]), &(actions[place*2+1]), SCALE, A, B, C, D, &env);
      } else {
	int64_t done = 0;
	while(!done) {
	  uint64_t source = dxor128(&env) * (ne + a_count);
	  if(source < ne) {
	    actions[place*2] = ~src[source];
	    actions[place*2+1] = ~dst[source];
	    done = 1;
	  } else {
	    source -= ne;
	    if(actions[source*2] > 0) {
	      actions[place*2] = ~actions[source*2];
	      actions[place*2+1] = ~actions[source*2+1];
	      done = 1;
	    }
	  }
	}
      }
    }
  }
  printf("%18s : %lf,\n", "Generate actions", toc());

  /* Write actions file */
  tic();
  NULLCHECK(fp_actions = fopen(actions_file, "w"));

  ZEROCHECK(fwrite(&endian_check, sizeof(uint64_t), 1, fp_actions));
  ZEROCHECK(fwrite(&NUM_ACTIONS, sizeof(uint64_t), 1, fp_actions));
  ZEROCHECK(fwrite(actions, sizeof(uint64_t), NUM_ACTIONS * 2, fp_actions));

  fclose(fp_actions);
  printf("%18s : %lf,\n", "Write actions", toc());
}

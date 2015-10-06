#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/x86_full_empty.h"
#include "compat/luc.h"

#ifdef __APPLE__
#define MAP_ANONYMOUS MAP_ANON
#endif

/* A bucket sort */
void
SortStart (int64_t NV, int64_t NE, int64_t * sv1, int64_t * ev1, int64_t * w1, int64_t * sv2, int64_t * ev2, int64_t * w2, int64_t * start)
{
  int64_t i;

  OMP("omp parallel for")
  for (i = 0; i < NV + 2; i++) start[i] = 0;

  start += 2;

  /* Histogram key values */
  OMP("omp parallel for")
  MTA("mta assert no alias *start *sv1")
  for (i = 0; i < NE; i++) stinger_int64_fetch_add(&start[sv1[i]], 1);

  /* Compute start index of each bucket */
  for (i = 1; i < NV; i++) start[i] += start[i-1];

  start --;

  /* Move edges into its bucket's segment */
  OMP("omp parallel for")
  MTA("mta assert no dependence")
  for (i = 0; i < NE; i++) {
    int64_t index = stinger_int64_fetch_add(&start[sv1[i]], 1);
    sv2[index] = sv1[i];
    ev2[index] = ev1[i];
    w2[index] = w1[i];
  }
}

static inline char *
remove_comments (char *filemem, int64_t stop)
{
  char buffer[1000];

  while ( sscanf(filemem, "%s\n", buffer) )
  {
    if (buffer[0] == stop) break;
    filemem = strchr(filemem+1, '\n');
  }
  return filemem;
}

static inline char *
nextchar (char * buffer, char c)
{
  while(*(buffer++) != c && (*buffer) != 0);
  return buffer;
}

static inline char *
readint (char * buffer, int64_t * n)
{
  int64_t ans = 0;
  while(*buffer != '\n' && *buffer != ' ' && *buffer != 0 )
  {
    ans *= 10;
    ans += (int64_t) (buffer[0] - '0');
    buffer++;
  }
  buffer++;
  *n = ans;
  return buffer;
}

static inline char *
readline (char * buffer, int64_t * start, int64_t * end, int64_t * weight)
{
  buffer = nextchar(buffer, ' ');
  buffer = readint(buffer, start);
  buffer = readint(buffer, end);
  buffer = readint(buffer, weight);
  return buffer;
}


int
load_dimacs_graph (struct stinger * S, const char * filename)
{
  int64_t i;
  int64_t NE, NV;
#if !defined(__MTA__)
  off_t filesize;
#else
  size_t filesize;
#endif

  /* initialize the IO subsystem */
  luc_io_init();

  /* get the file size on disk */
  luc_stat(filename, &filesize);
  //printf("Filesize: %ld\n", (int64_t)filesize);

  /* allocate enough memory to store the entire file */
  char * filemem = (char *) xmmap (NULL, (filesize + 5) * sizeof (char),
      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0); 
  filemem[filesize] = 0;
  filemem[filesize+1] = 0;
  filemem[filesize+2] = 0;
  filemem[filesize+3] = 0;

  /* load the file from disk into memory completely */
  luc_snapin (filename, filemem, filesize);
  char *ptr = filemem;
  char buffer[100];

  /* skip any comments at the top */
  ptr = remove_comments(ptr, 'p');

  /* the first real line must be a header that states the number of vertices and edges */
  sscanf(ptr, "%s %*s %ld %ld\n", buffer, &NV, &NE);
  //printf("NV: %ld, NE: %ld\n", NV, NE);
  ptr = strchr(ptr, '\n');

  /* skip any intermediate comments */
  ptr = remove_comments(ptr, 'a');

  /* allocate arrays to hold the edges */
  int64_t *sV1 = xmalloc(NE * sizeof(int64_t));
  int64_t *eV1 = xmalloc(NE * sizeof(int64_t));
  int64_t *w1 = xmalloc(NE * sizeof(int64_t));
  int64_t *sV2 = xmalloc(NE * sizeof(int64_t));
  int64_t *eV2 = xmalloc(NE * sizeof(int64_t));
  int64_t *w2 = xmalloc(NE * sizeof(int64_t));
  int64_t *edgeStart = xmalloc((NV+2) * sizeof(int64_t));

  int64_t edgecount = 0;

#if defined(__MTA__)
  /* split the file into non-overlapping workloads and parallelize among threads */
  MTA("mta assert parallel")
    MTA("mta dynamic schedule")
    MTA("mta use 100 streams")
    for (i = 0; i < 5000; i++)
    {
      int64_t startV, endV, weight;
      char *myPtr = ptr + (i*(filesize/5000));
      char *endPtr = ptr + (i+1)*(filesize/5000);
      if(i == 4999) endPtr = filemem+filesize-1;
      if (myPtr > filemem+filesize-1) myPtr = endPtr+1;
      myPtr = nextchar(myPtr, '\n');
      while ( myPtr <= endPtr )
      {
	myPtr = readline(myPtr,	&startV, &endV, &weight);
	int64_t k = stinger_int64_fetch_add(&edgecount, 1);
	sV1[k] = startV;
	eV1[k] = endV;
	w1[k] = weight;
      }
    }
#else
  /* we don't do this in parallel */
  {
    int64_t startV, endV, weight;
    char *myPtr = ptr;
    char *endPtr = filemem+filesize-1;
    myPtr = nextchar(myPtr, '\n');
    while ( myPtr <= endPtr )
    {
      myPtr = readline(myPtr, &startV, &endV, &weight);
      int64_t k = edgecount++;
      sV1[k] = startV;
      eV1[k] = endV;
      w1[k] = weight;
    }
  }
#endif

  //printf("edges ingested: %ld\n", edgecount);

  /* done with the file */
  munmap (filemem, (filesize + 5) * sizeof (char));

  /* sanity check */
  if (edgecount != NE) {
    perror("Incorrect number of edges");
    exit(-1);
  }

  /* Here we are going to attempt to figure out if the file
   * is 0-indexed or 1-indexed because the DIMACS "specification"
   * is unspecified. */
#define CHECK_EXTREME(a,OP,X) \
  do { if (a OP X) { int64_t X2 = readfe (&X); if (a OP X2) X2 = a; writeef (&X, X2); } } while (0)

  int64_t minv = 2*NV, maxv = 0;
  MTA("mta assert nodep")
    for (i = 0; i < NE; ++i) {
      const int64_t sv = sV1[i], ev = eV1[i];
      if (sv < ev) {
	CHECK_EXTREME(sv, <, minv);
	CHECK_EXTREME(ev, >, maxv);
      } else {
	CHECK_EXTREME(ev, <, minv);
	CHECK_EXTREME(sv, >, maxv);
      }
    }
  /* Check if the graph may have been 1-based, adjust if possible. */
  if (0 < minv && maxv == NV) {
    --minv;
    --maxv;
    for (i = 0; i < NE; ++i) {
      --sV1[i];
      --eV1[i];
    }
  }
  if (maxv >= NV) {
    perror ("Vertices out of range");
    exit(-1);
  }

  /* bucket sort the edges so that we can build the graph */
  SortStart (NV, NE, sV1, eV1, w1, sV2, eV2, w2, edgeStart);

  /* build up the STINGER graph */
  OMP("omp parallel for")
  for (int64_t i = 0; i < NE; i++) {
    stinger_insert_edge (S, 0, sV2[i], eV2[i], w2[i], 0);
  }


  free(sV1);
  free(eV1);
  free(w1);
  free(sV2);
  free(eV2);
  free(w2);
  free(edgeStart);

  return NE;
}

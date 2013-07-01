extern "C" {
  #include "stinger_core/stinger.h"
  #include "stinger_utils/timer.h"
  #include "stinger_core/xmalloc.h"
}

#include "proto/stinger-batch.pb.h"
#include "send_rcv.h"

#if !defined(MTA)
#define MTA(x)
#endif

#if defined(__GNUC__)
#define FN_MAY_BE_UNUSED __attribute__((unused))
#else
#define FN_MAY_BE_UNUSED
#endif

#include <cstdio>
#include <algorithm>
#include <limits>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace gt::stinger;

static inline char
ascii_tolower (char x)
{
  if (x <= 'Z' && x >= 'A')
    return x - ('Z'-'z');
  return x;
}

template <class T>
void
src_string (const T& in, std::string& out)
{
  out = in.source_str();
  std::transform(out.begin(), out.end(), out.begin(), ascii_tolower);
}
template <class T>
void
dest_string (const T& in, std::string& out)
{
  out = in.destination_str();
  std::transform(out.begin(), out.end(), out.begin(), ascii_tolower);
}

static bool dropped_vertices = false;

//struct community_state cstate;
int64_t n_components, n_nonsingleton_components, max_compsize;
int64_t min_batch_ts, max_batch_ts;
double processing_time, spdup;
static int64_t * comp_vlist;
static int64_t * comp_mark;

MTA("mta inline")
MTA("mta expect parallel")
static inline int64_t
int64_fetch_add (int64_t * p, int64_t incr)
{
#if defined(__MTA__)
  return int_fetch_add (p, incr);
#elif defined(_OPENMP)
#if defined(__GNUC__)
  return __sync_fetch_and_add (p, incr);
#elif defined(__INTEL_COMPILER)
  return __sync_fetch_and_add (p, incr);
#else
#error "Atomics not defined..."
#endif
#else
  int64_t out = *p;
  *p += incr;
  return out;
#endif
}

void
components_init(struct stinger * S, int64_t nv, int64_t * component_map);

void
components_batch(struct stinger * S, int64_t nv, int64_t * component_map);

#define V_A(X,...) do { fprintf(stdout, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__); fflush (stdout); } while (0)
#define V(X) V_A(X,NULL)

#define ACCUM_INCR do {							\
  int64_t where;							\
  stinger_incr_edge_pair(S, in.type(), u, v, in.weight(), in.time());	\
  where = int64_fetch_add (&nincr, 1);					\
  incr[3*where+0] = u;							\
  incr[3*where+1] = v;							\
  incr[3*where+2] = in.weight();					\
 } while (0)

#define ACCUM_REM do {					\
  int64_t where;					\
  stinger_remove_edge_pair(S, del.type(), u, v);	\
  where = int64_fetch_add (&nrem, 1);			\
  rem[2*where+0] = u;					\
  rem[2*where+1] = v;					\
 } while (0)

/**
* @brief Inserts and removes the edges contained in a batch.
*
* Maps new vertices as needed for insertions.  Deletions with non-existant vertices
* are ignored.
*
* @param S A pointer to the STINGER structure.
* @param batch A reference to the protobuf
*
* @return 0 on success.
*/
int
process_batch(stinger_t * S, StingerBatch & batch,
	      struct community_state * cstate)
{
  int64_t nincr = 0, nrem = 0;
  int64_t * incr = NULL;
  int64_t * rem = NULL;

  incr = (int64_t*)xmalloc ((3 * batch.insertions_size () + 2 * batch.deletions_size ()) * sizeof (*incr));
  rem = &incr[3 * batch.insertions_size ()];

  min_batch_ts = std::numeric_limits<int64_t>::max();
  max_batch_ts = 0;

#define TS(ea_) do { const int64_t ts = (ea_).time(); if (ts > mxts) mxts = ts; if (ts < mnts) mnts = ts; } while (0)

  OMP("omp parallel") {
    int64_t mxts = 0, mnts = std::numeric_limits<int64_t>::max();
    std::string src, dest; /* Thread-local buffers */
    switch (batch.type ()) {
    case NUMBERS_ONLY:
      OMP("omp for")
	for (size_t i = 0; i < batch.insertions_size(); i++) {
	  const EdgeInsertion & in = batch.insertions(i);
	  const int64_t u = in.source ();
	  const int64_t v = in.destination ();
	  TS(in);
	  if (u < STINGER_MAX_LVERTICES && v < STINGER_MAX_LVERTICES)
	    ACCUM_INCR;
	  else {
	    OMP("omp critical")
	      if (!dropped_vertices) {
		V ("Dropped vertices...");
		dropped_vertices = true;
	      }
	  }
	}
      assert (dropped_vertices || nincr == batch.insertions_size ());

      OMP("omp for")
	for(size_t d = 0; d < batch.deletions_size(); d++) {
	  const EdgeDeletion & del = batch.deletions(d);
	  const int64_t u = del.source ();
	  const int64_t v = del.destination ();
	  //TS(del);
	  if (u < STINGER_MAX_LVERTICES && v < STINGER_MAX_LVERTICES)
	    ACCUM_REM;
	  else {
	    OMP("omp critical")
	      if (!dropped_vertices) {
		V ("Dropped vertices...");
		dropped_vertices = true;
	      }
	  }
	}
      assert (dropped_vertices || nrem == batch.deletions_size ());
      break;

    case STRINGS_ONLY:
      OMP("omp for")
	for (size_t i = 0; i < batch.insertions_size(); i++) {
	  const EdgeInsertion & in = batch.insertions(i);
	  int64_t u, v;
	  TS(in);
	  src_string (in, src);
	  dest_string (in, dest);
	  stinger_mapping_create(S, src.c_str(), src.length(), &u);
	  stinger_mapping_create(S, dest.c_str(), dest.length(), &v);
	  if (u < STINGER_MAX_LVERTICES && v < STINGER_MAX_LVERTICES)
	    ACCUM_INCR;
	  else {
	    OMP("omp critical")
	      if (!dropped_vertices) {
		V ("Dropped vertices...");
		dropped_vertices = true;
	      }
	  }
	}
      assert (dropped_vertices || nincr == batch.insertions_size ());

      OMP("omp for")
	for(size_t d = 0; d < batch.deletions_size(); d++) {
	  const EdgeDeletion & del = batch.deletions(d);
	  int64_t u, v;
	  //TS(del);
	  src_string (del, src);
	  dest_string (del, dest);
	  u = stinger_mapping_lookup(S, src.c_str(), src.length());
	  v = stinger_mapping_lookup(S, dest.c_str(), dest.length());

	  if(u != -1 && v != -1) {
	    if (u < STINGER_MAX_LVERTICES && v < STINGER_MAX_LVERTICES)
	      ACCUM_REM;
	    else {
	      OMP("omp critical")
		if (!dropped_vertices) {
		  V ("Dropped vertices...");
		  dropped_vertices = true;
		}
	    }
	  }
	}
      assert (dropped_vertices || nrem == batch.deletions_size ());
      break;

    case MIXED:
      OMP("omp for")
	for (size_t i = 0; i < batch.insertions_size(); i++) {
	  const EdgeInsertion & in = batch.insertions(i);
	  int64_t u, v;
	  TS(in);
	  if (in.has_source())
	    u = in.source();
	  else {
	    src_string (in, src);
	    stinger_mapping_create (S, src.c_str(), src.length(), &u);
	  }

	  if (in.has_destination())
	    v = in.destination();
	  else {
	    dest_string (in, src);
	    stinger_mapping_create(S, dest.c_str(), dest.length(), &v);
	  }

	  if (u < STINGER_MAX_LVERTICES && v < STINGER_MAX_LVERTICES)
	    ACCUM_INCR;
	  else {
	    OMP("omp critical")
	      if (!dropped_vertices) {
		V ("Dropped vertices...");
		dropped_vertices = true;
	      }
	  }
	}
      assert (dropped_vertices || nincr == batch.insertions_size ());

      OMP("omp for")
	for(size_t d = 0; d < batch.deletions_size(); d++) {
	  const EdgeDeletion & del = batch.deletions(d);
	  int64_t u, v;
	  //TS(del);

	  if (del.has_source())
	    u = del.source();
	  else {
	    src_string (del, src);
	    u = stinger_mapping_lookup(S, src.c_str(), src.length());
	  }

	  if (del.has_destination())
	    v = del.destination();
	  else {
	    dest_string (del, dest);
	    v = stinger_mapping_lookup(S, dest.c_str(), dest.length());
	  }

	  if(u != -1 && v != -1) {
	    if (u < STINGER_MAX_LVERTICES && v < STINGER_MAX_LVERTICES)
	      ACCUM_REM;
	    else {
	      OMP("omp critical")
		if (!dropped_vertices) {
		  V ("Dropped vertices...");
		  dropped_vertices = true;
		}
	    }
	  }
	}
      assert (dropped_vertices || nrem == batch.deletions_size ());
      break;

    default:
      abort ();
    }

    OMP("omp critical") {
      if (mxts > max_batch_ts) max_batch_ts = mxts;
      if (mnts < min_batch_ts) min_batch_ts = mnts;
    }
  }

#undef TS

//  cstate_preproc (cstate, S, nincr, incr, nrem, rem);

  free (incr);
  return 0;
}

static void
split_day_in_year (int diy, int * month, int * dom)
{
  int mn = 0;
#define OUT do { *month = mn; *dom = diy; return; } while (0)
#define MONTH(d_) if (diy < (d_)) OUT; diy -= (d_); ++mn
  MONTH(31);
  MONTH(28); 
  MONTH(31);
  MONTH(30);
  MONTH(31);
  MONTH(30);
  MONTH(31);
  MONTH(31);
  MONTH(30);
  MONTH(31);
  MONTH(30);
  MONTH(31);
  assert (0);
  *month = -1;
  *dom = -1;
}

static const char *
month_name[12] = { "Jan", "Feb", "Mar", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static void
ts_to_str (int64_t ts_in, char * out, size_t len)
{
  int64_t ts = ts_in;
  ts += (31 + 28)*24*3600; /* XXX: Sample starts in March. */
  ts -= 7*3600; /* XXX: Pacific is seven hours behind UTC. */
  const bool pst =
    (ts < (2*3600 + (9 + 31+28)*24*3600)) &&
    (ts < (2*3600 + (3 + 31+28+31+30+31+30+31+31+30+31)*24*3600))
	 ; /* 2am 10 Mar to 2am 3 Nov */
  if (pst) ts += 3600;
  const int sec = ts%60;
  const int min = (ts/60)%60;
  const int hr = (ts/(60*60))%24;
  const int day_in_year = ts_in / (60*60*24);
  int month, dom;
  split_day_in_year (day_in_year, &month, &dom);
  snprintf (out, len, "%2d:%02d:%02d P%cT %d %s 2013",
	    hr, min, sec,
	    (pst? 'S' : 'D'),
	    dom, month_name[month]);
}

int
main(int argc, char *argv[])
{
  struct stinger * S = stinger_new();

  /* register edge and vertex types */
  int64_t vtype_twitter_handle;
  stinger_vtype_names_create_type(S, "TwitterHandle", &vtype_twitter_handle);

  int64_t etype_mention;
  stinger_vtype_names_create_type(S, "Mention", &etype_mention);

//  web_start_stinger(S, "8088");

  /* global options */
  int port = 10101;
  uint64_t buffer_size = 1ULL << 28ULL;

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:b:"))) {
    switch(opt) {
      case 'p': {
	port = atoi(optarg);
      } break;

      case 'b': {
	buffer_size = atol(optarg);
      } break;

      case '?':
      case 'h': {
	printf("Usage:    %s [-p port] [-b buffer_size]\n", argv[0]);
	printf("Defaults: port: %d buffer_size: %lu\n", port, (unsigned long) buffer_size);
	exit(0);
      } break;

    }
  }

  int sock_handle;
  struct sockaddr_in sock_addr = { AF_INET, (in_port_t)port, 0};

  if(-1 == (sock_handle = socket(AF_INET, SOCK_STREAM, 0))) {
    perror("Socket create failed.\n");
    exit(-1);
  }

  if(-1 == bind(sock_handle, (const sockaddr *)&sock_addr, sizeof(sock_addr))) {
    perror("Socket bind failed.\n");
    exit(-1);
  }

  if(-1 == listen(sock_handle, 1)) {
    perror("Socket listen failed.\n");
    exit(-1);
  }

  const char * buffer = (char *)malloc(buffer_size);
  if(!buffer) {
    perror("Buffer alloc failed.\n");
    exit(-1);
  }

  int64_t * components = (int64_t *)xmalloc(sizeof(int64_t) * STINGER_MAX_LVERTICES);
  comp_vlist = (int64_t *)xmalloc(sizeof(int64_t) * STINGER_MAX_LVERTICES);
  comp_mark = (int64_t *)xmalloc(sizeof(int64_t) * STINGER_MAX_LVERTICES);
  OMP("omp parallel for")
  for (int64_t k = 0; k < STINGER_MAX_LVERTICES; ++k)
    comp_mark[k] = -1;

//  init_empty_community_state (&cstate, STINGER_MAX_LVERTICES, 2*STINGER_MAX_LVERTICES);

/*
  web_set_label_container("ConnectedComponentIDs", components);
  web_set_label_container("CommunityIDs", cstate.cmap);
  web_set_score_container("BetweennessCentralities", centralities);
*/
  components_init(S, STINGER_MAX_LVERTICES, components);

  V_A("STINGER server listening for input on port %d, web server on port 8088.",
      (int)port);

  while(1) {
    int accept_handle = accept(sock_handle, NULL, NULL);
    int nfail = 0;

    V("Ready to accept messages.");
    while(1) {

      StingerBatch batch;
      if(recv_message(accept_handle, batch)) {
	nfail = 0;

	V_A("Received message of size %ld", (long)batch.ByteSize());

	if (0 == batch.insertions_size () && 0 == batch.deletions_size ()) {
	  V("Empty batch.");
	  if (!batch.keep_alive ())
	    break;
	  else
	    continue;
	}

	double processing_time_start;
	//batch.PrintDebugString();

//	processing_time_start = timer ();

	//process_batch(S, batch, &cstate);
	process_batch(S, batch, NULL);

	components_batch(S, STINGER_MAX_LVERTICES, components);

//	cstate_update (&cstate, S);

//	processing_time = timer () - processing_time_start;
//	spdup = (max_batch_ts - min_batch_ts) / processing_time;

	{
	  char mints[100], maxts[100];

	  ts_to_str (min_batch_ts, mints, sizeof (mints)-1);
	  ts_to_str (max_batch_ts, maxts, sizeof (maxts)-1);
	  V_A("Time stamp range: %s ... %s", mints, maxts);
	}

	V_A("Number of non-singleton components %ld/%ld, max size %ld",
	    (long)n_nonsingleton_components, (long)n_components,
	    (long)max_compsize);

/*	V_A("Number of non-singleton communities %ld/%ld, max size %ld, modularity %g",
	    (long)cstate.n_nonsingletons, (long)cstate.cg.nv,
	    (long)cstate.max_csize,
	    cstate.modularity);*/

	V_A("Total time: %ld, processing time: %g, speedup %g",
	    (long)(max_batch_ts-min_batch_ts), processing_time, spdup);

	if(!batch.keep_alive())
	  break;

      } else {
	++nfail;
	V("ERROR Parsing failed.\n");
	if (nfail > 2) break;
      }
    }
    if (nfail > 2) break;
  }

  return 0;
}

void
components_init(struct stinger * S, int64_t nv, int64_t * component_map) {
  /* Initialize each vertex with its own component label in parallel */
  OMP ("omp parallel for")
    for (uint64_t i = 0; i < nv; i++) {
      component_map[i] = i;
    }
  
  /* S is empty... components_batch(S, nv, component_map); */
  n_components = nv;
}

void
components_batch(struct stinger * S, int64_t nv, int64_t * component_map) {
  int64_t nc;
  /* Iterate until no changes occur */
  while (1) {
    int changed = 0;

    /* For all edges in the STINGER graph of type 0 in parallel, attempt to assign
       lesser component IDs to neighbors with greater component IDs */
    for(uint64_t t = 0; t < STINGER_NUMETYPES; t++) {
      STINGER_PARALLEL_FORALL_EDGES_BEGIN (S, t) {
	if (component_map[STINGER_EDGE_DEST] <
	    component_map[STINGER_EDGE_SOURCE]) {
	  component_map[STINGER_EDGE_SOURCE] = component_map[STINGER_EDGE_DEST];
	  changed++;
	}
      }
      STINGER_PARALLEL_FORALL_EDGES_END ();
    }

    /* if nothing changed */
    if (!changed)
      break;

    /* Tree climbing with OpenMP parallel for */
    OMP ("omp parallel for")
      MTA ("mta assert nodep")
      for (uint64_t i = 0; i < nv; i++) {
	while (component_map[i] != component_map[component_map[i]])
          component_map[i] = component_map[component_map[i]];
      }
  }
  nc = 0;
  OMP ("omp parallel for reduction(+: nc)")
    for (uint64_t i = 0; i < nv; i++)
      if (component_map[i] == component_map[component_map[i]]) ++nc;
  n_components = nc;

  n_nonsingleton_components = 0;
  max_compsize = 0;
  int64_t nnsc = 0, mxcsz = 0;
  if (nc) {
    int64_t nvlist = 0;
    int64_t * vlist = comp_vlist;
    int64_t * mark = comp_mark;
    OMP("omp parallel") {
      OMP("omp for")
	for (int64_t k = 0; k < nv; ++k) {
	  const int64_t c = component_map[k];
	  assert (c < n_components);
	  assert (c >= 0);
	  int64_t subcnt = int64_fetch_add (&mark[c], 1);
	  if (-1 == subcnt) { /* First to claim. */
	    int64_t where = int64_fetch_add (&nvlist, 1);
	    assert (where < n_components);
	    assert (where >= 0);
	    vlist[where] = c;
	  }
	}

      int64_t cmaxsz = -1;
      OMP("omp for reduction(+: nnsc)")
	for (int64_t k = 0; k < nvlist; ++k) {
	  const int64_t c = vlist[k];
	  assert (c < n_components);
	  assert (c >= 0);
	  const int64_t csz = mark[c]+1;
	  if (csz > 1) ++nnsc;
	  if (csz > cmaxsz) cmaxsz = csz;
	  mark[c] = -1; /* Reset to -1. */
	}
      OMP("omp critical")
	if (cmaxsz > mxcsz) mxcsz = cmaxsz;

#if !defined(NDEBUG)
      OMP("omp for") for (int64_t k = 0; k < n_components; ++k) assert (mark[k] == -1);
#endif
    }
    /* XXX: not right, but cannot find where these are appearing as
       zero in the stats display... */
    if (nnsc)
      n_nonsingleton_components = nnsc;
    if (mxcsz)
      max_compsize = mxcsz;
  }
}


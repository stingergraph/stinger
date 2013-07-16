#include <cstdio>
#include <algorithm>
#include <limits>
#include <unistd.h>

extern "C" {
  #include "stinger_core/stinger.h"
  #include "stinger_core/stinger_atomics.h"
  #include "stinger_core/xmalloc.h"
}

#include "proto/stinger-batch.pb.h"
#include "server.h"
#include "send_rcv.h"


#define ACCUM_INCR do {							\
  int64_t where;							\
  stinger_incr_edge_pair(S, in.type(), u, v, in.weight(), in.time());	\
  where = stinger_int64_fetch_add (&nincr, 1);				\
  incr[3*where+0] = u;							\
  incr[3*where+1] = v;							\
  incr[3*where+2] = in.weight();					\
 } while (0)

#define ACCUM_REM do {					\
  int64_t where;					\
  stinger_remove_edge_pair(S, del.type(), u, v);	\
  where = stinger_int64_fetch_add (&nrem, 1);		\
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

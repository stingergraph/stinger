#include <cstdio>
#include <algorithm>
#include <limits>
#include <unistd.h>

#include "stinger_net/stinger_server_state.h"

extern "C" {
#include "stinger_core/stinger.h"
#include "stinger_core/stinger_error.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
}

using namespace gt::stinger;

#include "server.h"

template<int64_t type>
inline void
handle_vertex_names_types(VertexUpdate & vup, stinger_t * S)
{
  std::string src;
  int64_t u;

  if(type == STRINGS_ONLY || (type == MIXED && vup.has_vertex_str())) {
    vertex_string(vup, src);
    stinger_mapping_create(S, src.c_str(), src.length(), &u);
    vup.set_vertex(u);
  } else {
    u = vup.vertex();
  }

  if(vup.has_type_str()) {
    int64_t vtype = 0;
    if(-1 == stinger_vtype_names_create_type(S, vup.type_str().c_str(), &vtype)) {
      LOG_E_A("Error creating vertex type %s", vup.type_str().c_str());
      vtype = 0;
    }
    vup.set_type(vtype);
  }

  if(vup.has_type()) {
    stinger_vtype_set(S, vup.vertex(), vup.type());
  }

  if(vup.has_set_weight()) {
    stinger_vweight_set(S, vup.vertex(), vup.set_weight());
  }

  if(vup.has_incr_weight()) {
    stinger_vweight_increment_atomic(S, vup.vertex(), vup.incr_weight());
  }
}

template<int64_t type>
inline void
handle_edge_names_types(EdgeInsertion & in, stinger_t * S, std::string & src, std::string & dest, int64_t & u, int64_t & v)
{
  if(type == STRINGS_ONLY) {
    src_string (in, src);
    dest_string (in, dest);
    stinger_mapping_create(S, src.c_str(), src.length(), &u);
    stinger_mapping_create(S, dest.c_str(), dest.length(), &v);
    in.set_source(u);
    in.set_destination(v);
  }

  if(type == MIXED) {
    if (in.has_source()) {
      u = in.source();
      char * name = NULL;
      uint64_t name_len = 0;
      if(-1 != stinger_mapping_physid_direct(S, u, &name, &name_len))
	in.set_source_str(name, name_len);
      else
	in.set_source_str("");

    } else {
      src_string (in, src);
      if(src.length())
      stinger_mapping_create (S, src.c_str(), src.length(), &u);
      if(u != -1) in.set_source(u);
    }

    if (in.has_destination()) {
      v = in.destination();
      char * name = NULL;
      uint64_t name_len = 0;
      if(-1 != stinger_mapping_physid_direct(S, v, &name, &name_len))
	in.set_destination_str(name, name_len);
      else
	in.set_destination_str("");
    } else {
      dest_string (in, dest);
      if(dest.length())
      stinger_mapping_create(S, dest.c_str(), dest.length(), &v);
      if(v != -1) in.set_destination(v);
    }
  }

  if(in.has_type_str()) {
    int64_t etype = 0;
    if(-1 == stinger_etype_names_create_type(S, in.type_str().c_str(), &etype)) {
      LOG_E_A("Error creating edge type %s", in.type_str().c_str());
      etype = 0;
    }
    in.set_type(etype);
  }
}

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
process_batch(stinger_t * S, StingerBatch & batch)
{
  StingerServerState & server_state = StingerServerState::get_server_state();
  min_batch_ts = std::numeric_limits<int64_t>::max();
  max_batch_ts = std::numeric_limits<int64_t>::min();
  int64_t chunksize = 8192;

#define TS(ea_) do { const int64_t ts = (ea_).time(); if (ts > mxts) mxts = ts; if (ts < mnts) mnts = ts; } while (0)

  OMP("omp parallel") {
    int64_t mxts = 0, mnts = std::numeric_limits<int64_t>::max();
    std::string src, dest; /* Thread-local buffers */
    switch (batch.type ()) {

      case NUMBERS_ONLY: {
	if(server_state.convert_numbers_only_to_strings()) {
	  OMP("omp parallel for schedule(dynamic, chunksize)")
	    for (size_t i = 0; i < batch.insertions_size(); i++) {
	      EdgeInsertion & in = *batch.mutable_insertions(i);
	      int64_t u, v;
	      TS(in);
	      handle_edge_names_types<NUMBERS_ONLY>(in, S, src, dest, u, v);
	      if(batch.make_undirected()) {
		in.set_result(stinger_incr_edge_pair(S, in.type(), in.source(), in.destination(), in.weight(), in.time()));
	      } else {
		in.set_result(stinger_incr_edge(S, in.type(), in.source(), in.destination(), in.weight(), in.time()));
	      }
	      if(in.result() == -1) {
		LOG_E_A("Error inserting edge <%ld, %ld>", in.source(), in.destination());
	      } else {
		char * name = NULL;
		uint64_t name_len = 0;
		if(-1 != stinger_mapping_physid_direct(S, in.source(), &name, &name_len))
		  in.set_source_str(name, name_len);
		else
		  in.set_source_str("");

		name = NULL;
		name_len = 0;
		if(-1 != stinger_mapping_physid_direct(S, in.destination(), &name, &name_len))
		  in.set_destination_str(name, name_len);
		else
		  in.set_destination_str("");
	      }
	    }

	  OMP("omp parallel for schedule(dynamic, chunksize)")
	    for(size_t d = 0; d < batch.deletions_size(); d++) {
	      EdgeDeletion & del = *batch.mutable_deletions(d);
	      if(batch.make_undirected()) {
		del.set_result(stinger_remove_edge_pair(S, del.type(), del.source(), del.destination()));
	      } else {
		del.set_result(stinger_remove_edge(S, del.type(), del.source(), del.destination()));
	      }
	      if(-1 == del.result()) {
		LOG_E_A("Error removing edge <%ld, %ld>", del.source(), del.destination());
	      } else {
		char * name = NULL;
		uint64_t name_len = 0;
		if(-1 != stinger_mapping_physid_direct(S, del.source(), &name, &name_len))
		  del.set_source_str(name, name_len);
		else
		  del.set_source_str("");

		name = NULL;
		name_len = 0;
		if(-1 != stinger_mapping_physid_direct(S, del.destination(), &name, &name_len))
		  del.set_destination_str(name, name_len);
		else
		  del.set_destination_str("");
	      }
	    }
	} else {
	  OMP("omp parallel for schedule(dynamic, chunksize)")
	    for (size_t i = 0; i < batch.insertions_size(); i++) {
	      EdgeInsertion & in = *batch.mutable_insertions(i);
	      int64_t u, v;
	      TS(in);
	      handle_edge_names_types<NUMBERS_ONLY>(in, S, src, dest, u, v);
	      if(batch.make_undirected()) {
		in.set_result(stinger_incr_edge_pair(S, in.type(), in.source(), in.destination(), in.weight(), in.time()));
	      } else {
		in.set_result(stinger_incr_edge(S, in.type(), in.source(), in.destination(), in.weight(), in.time()));
	      }
	      if(in.result() == -1) {
		LOG_E_A("Error inserting edge <%ld, %ld>", in.source(), in.destination());
	      }
	    }

	  OMP("omp parallel for schedule(dynamic, chunksize)")
	    for(size_t d = 0; d < batch.deletions_size(); d++) {
	      EdgeDeletion & del = *batch.mutable_deletions(d);
	      if(batch.make_undirected()) {
		del.set_result(stinger_remove_edge_pair(S, del.type(), del.source(), del.destination()));
	      } else {
		del.set_result(stinger_remove_edge(S, del.type(), del.source(), del.destination()));
	      }
	      if(-1 == del.result()) {
		LOG_E_A("Error removing edge <%ld, %ld>", del.source(), del.destination());
	      }
	    }
	}

	OMP("omp for")
	  for(size_t d = 0; d < batch.vertex_updates_size(); d++) {
	    VertexUpdate & vup = *batch.mutable_vertex_updates(d);
	    handle_vertex_names_types<NUMBERS_ONLY>(vup, S);
	  }
      } break;

      case STRINGS_ONLY:
	OMP("omp parallel for schedule(dynamic, chunksize)")
	  for (size_t i = 0; i < batch.insertions_size(); i++) {
	    EdgeInsertion & in = *batch.mutable_insertions(i);
	    int64_t u, v;

	    TS(in);
	    handle_edge_names_types<STRINGS_ONLY>(in, S, src, dest, u, v);

	    if(u != -1 && v != -1) {
	      if(batch.make_undirected()) {
		in.set_result(stinger_incr_edge_pair(S, in.type(), u, v, in.weight(), in.time()));
	      } else {
		in.set_result(stinger_incr_edge(S, in.type(), u, v, in.weight(), in.time()));
	      }
	      if(in.result() == -1) {
	      LOG_E_A("Error inserting edge <%s, %s>", in.source_str().c_str(), in.destination_str().c_str());
	      } else {
		in.set_source(u); in.set_destination(v);
	      }
	    }
	  }

	OMP("omp parallel for schedule(dynamic, chunksize)")
	  for(size_t d = 0; d < batch.deletions_size(); d++) {
	    EdgeDeletion & del = *batch.mutable_deletions(d);
	    int64_t u, v;

	    src_string (del, src);
	    dest_string (del, dest);
	    u = stinger_mapping_lookup(S, src.c_str(), src.length());
	    v = stinger_mapping_lookup(S, dest.c_str(), dest.length());

	    if(u != -1 && v != -1) {
	      if(batch.make_undirected()) {
		del.set_result(-1 == stinger_remove_edge_pair(S, del.type(), u, v));
	      } else {
		del.set_result(-1 == stinger_remove_edge(S, del.type(), u, v));
	      }
	      if(del.result() == -1) {
		LOG_E_A("Error removing edge <%s, %s>", del.source_str().c_str(), del.destination_str().c_str());
	      } else {
		del.set_source(u); del.set_destination(v);
	      }
	    }
	  }

	OMP("omp for")
	  for(size_t d = 0; d < batch.vertex_updates_size(); d++) {
	    VertexUpdate & vup = *batch.mutable_vertex_updates(d);
	    handle_vertex_names_types<STRINGS_ONLY>(vup, S);
	  }
	break;

      case MIXED:
	OMP("omp parallel for schedule(dynamic, chunksize)")
	  for (size_t i = 0; i < batch.insertions_size(); i++) {
	    EdgeInsertion & in = *batch.mutable_insertions(i);
	    int64_t u = -1, v = -1;
	    TS(in);
	    handle_edge_names_types<MIXED>(in, S, src, dest, u, v);
	    if(u != -1 && v != -1) {
	      if(batch.make_undirected()) {
		in.set_result(stinger_incr_edge_pair(S, in.type(), u, v, in.weight(), in.time()));
	      } else {
		in.set_result(stinger_incr_edge(S, in.type(), u, v, in.weight(), in.time()));
	      }
	      if(in.result() == -1) {
		LOG_E_A("Error inserting edge <%ld - %s, %ld - %s>", u, in.source_str().c_str(), v, 
			in.destination_str().c_str());
	      }
	    }
	  }

	OMP("omp parallel for schedule(dynamic, chunksize)")
	  for(size_t d = 0; d < batch.deletions_size(); d++) {
	    EdgeDeletion & del = *batch.mutable_deletions(d);
	    int64_t u, v;

	    if (del.has_source()) {
	      u = del.source();
	      char * name = NULL;
	      uint64_t name_len = 0;
	      if(-1 != stinger_mapping_physid_direct(S, del.source(), &name, &name_len))
		del.set_source_str(name, name_len);
	      else
		del.set_source_str("");
	    } else {
	      src_string (del, src);
	      u = stinger_mapping_lookup(S, src.c_str(), src.length());
	      if(u != -1) del.set_source(u);
	    }

	    if (del.has_destination()) {
	      v = del.destination();
	      char * name = NULL;
	      uint64_t name_len = 0;
	      if(-1 != stinger_mapping_physid_direct(S, del.destination(), &name, &name_len))
		del.set_destination_str(name, name_len);
	      else
		del.set_destination_str("");
	    } else {
	      dest_string (del, dest);
	      v = stinger_mapping_lookup(S, dest.c_str(), dest.length());
	      if(v != -1) del.set_destination(v);
	    }

	    if(u != -1 && v != -1) {
	      if(batch.make_undirected()) {
		del.set_result(stinger_remove_edge_pair(S, del.type(), u, v));
	      } else {
		del.set_result(stinger_remove_edge(S, del.type(), u, v));
	      }
	      if(del.result() == -1) {
		LOG_E_A("Error removing edge <%ld - %s, %ld - %s>", u, del.source_str().c_str(), v, 
			del.destination_str().c_str());
	      }
	    }
	  }

	OMP("omp for")
	  for(size_t d = 0; d < batch.vertex_updates_size(); d++) {
	    VertexUpdate & vup = *batch.mutable_vertex_updates(d);
	    handle_vertex_names_types<MIXED>(vup, S);
	  }
	break;

      default:
	abort ();
    }

    OMP("omp critical") {
      if (mxts > max_batch_ts) max_batch_ts = mxts;
      if (mnts < min_batch_ts) min_batch_ts = mnts;
    }
  }

  return 0;
}

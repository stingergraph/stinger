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
#include "stinger_core/stinger_batch_insert.h"

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

// Allows an EdgeInsertion to be passed to stinger_batch functions
struct EdgeInsertionAdapter
{
    typedef EdgeInsertion update;

    static int64_t get_type(const update &u) { return u.type(); }
    static void set_type(update &u, int64_t v) { u.set_type(v); }
    static int64_t get_source(const update &u) { return u.source(); }
    static void set_source(update &u, int64_t v) { u.set_source(v); }
    static int64_t get_dest(const update &u) { return u.destination(); }
    static void set_dest(update &u, int64_t v) { u.set_destination(v); }
    static int64_t get_weight(const update &u) { return u.weight(); }
    static int64_t get_time(const update &u) { return u.time(); }
    static int64_t get_result(const update& u) { return u.result(); }
    static int64_t set_result(update &u, int64_t v) { u.set_result(v); }
};

template <int64_t type>
void process_insertions(stinger_t * S, StingerBatch & batch)
{
    int64_t mxts = 0, mnts = std::numeric_limits<int64_t>::max();
    std::string src, dest;
    OMP("omp parallel for")
    for (size_t i = 0; i < batch.insertions_size(); i++)
    {
        EdgeInsertion & in = *batch.mutable_insertions(i);
        int64_t u = -1, v = -1;
        handle_edge_names_types<type>(in, S, src, dest, u, v);
        if(u == -1 || v == -1) {
            // Prevents batch update from trying to insert this edge
            in.set_result(-1);
        }
    }

    if (batch.make_undirected())
    {
        stinger_batch_incr_edge_pairs<EdgeInsertionAdapter>(
                S, batch.mutable_insertions()->begin(), batch.mutable_insertions()->end());
    } else {
        stinger_batch_incr_edges<EdgeInsertionAdapter>(
                S, batch.mutable_insertions()->begin(), batch.mutable_insertions()->end());
    }

    OMP("omp parallel for")
    for (size_t i = 0; i < batch.insertions_size(); i++)
    {
        EdgeInsertion & in = *batch.mutable_insertions(i);
        if (in.result() == -1)
        {
            switch (type)
            {
                case NUMBERS_ONLY:
                    LOG_E_A("Error inserting edge <%ld, %ld>", in.source(), in.destination());
                    break;
                case STRINGS_ONLY:
                    LOG_E_A("Error inserting edge <%s, %s>", in.source_str().c_str(), in.destination_str().c_str());
                    break;
                case MIXED:
                    LOG_E_A("Error inserting edge <%ld - %s, %ld - %s>",
                        in.source(), in.source_str().c_str(), in.destination(), in.destination_str().c_str());
                    break;
                default:
                    abort ();
            }
        } else if (type == NUMBERS_ONLY &&
                   StingerServerState::get_server_state().convert_numbers_only_to_strings())
        {
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
}


template <int64_t type>
void process_deletions(stinger_t * S, StingerBatch & batch){
    std::string src, dest;
    OMP("omp for")
    for(size_t d = 0; d < batch.deletions_size(); d++)
    {
        EdgeDeletion & del = *batch.mutable_deletions(d);
        // Look up src/dst ID from string, if necessary
        int64_t u, v;
        switch(type)
        {
            case NUMBERS_ONLY:
            {
                u = del.source();
                v = del.destination();
            } break;
            case STRINGS_ONLY:
            {
                src_string (del, src);
                dest_string (del, dest);
                u = stinger_mapping_lookup(S, src.c_str(), src.length());
                v = stinger_mapping_lookup(S, dest.c_str(), dest.length());
            } break;
            case MIXED:
            {
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
            } break;
            default:
                abort ();
        }

        if(u != -1 && v != -1) {
            // Do the deletion
            if(batch.make_undirected()) {
                del.set_result(stinger_remove_edge_pair(S, del.type(), del.source(), del.destination()));
            } else {
                del.set_result(stinger_remove_edge(S, del.type(), del.source(), del.destination()));
            }
            if (del.result() == -1) {
                switch (type)
                {
                    case NUMBERS_ONLY:
                        LOG_E_A("Error removing edge <%ld, %ld>", del.source(), del.destination());
                        break;
                    case STRINGS_ONLY:
                        LOG_E_A("Error removing edge <%s, %s>", del.source_str().c_str(), del.destination_str().c_str());
                        break;
                    case MIXED:
                        LOG_E_A("Error removing edge <%ld - %s, %ld - %s>",
                                u, del.source_str().c_str(), v, del.destination_str().c_str());
                        break;
                    default:
                        abort ();
                }
            } else if (type == NUMBERS_ONLY &&
                       StingerServerState::get_server_state().convert_numbers_only_to_strings())
            {
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
    }
}

template <int64_t type>
void process_vertex_updates(stinger_t * S, StingerBatch & batch){
    OMP("omp for")
    for(size_t d = 0; d < batch.vertex_updates_size(); d++) {
        VertexUpdate & vup = *batch.mutable_vertex_updates(d);
        handle_vertex_names_types<type>(vup, S);
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
    switch (batch.type ()) {
        case NUMBERS_ONLY:
            process_insertions<NUMBERS_ONLY>(S, batch);
            process_deletions<NUMBERS_ONLY>(S, batch);
            process_vertex_updates<NUMBERS_ONLY>(S, batch);
            break;
        case STRINGS_ONLY:
            process_insertions<STRINGS_ONLY>(S, batch);
            process_deletions<STRINGS_ONLY>(S, batch);
            process_vertex_updates<STRINGS_ONLY>(S, batch);
            break;
        case MIXED:
            process_insertions<MIXED>(S, batch);
            process_deletions<MIXED>(S, batch);
            process_vertex_updates<MIXED>(S, batch);
            break;
        default:
            abort();
    }

    return 0;
}

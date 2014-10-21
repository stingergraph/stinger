#include <climits>
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807 /* sometimes c++ just doesn't want this to work */
#endif

//#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

#include "stinger_core/xmalloc.h"
#include "stinger_utils/stinger_utils.h"
#include "rapidjson/document.h"
#include "json_rpc_server.h"
#include "json_rpc.h"
#include "session_handling.h"

using namespace gt::stinger;


static void
vlist_to_json_subgraph (stinger_t * S,
                        const int64_t nvlist, const int64_t * vlist, const int64_t * mark,
                        const bool strings,
                        const bool get_types,
                        const bool get_etypes,
                        const bool get_vtypes,
                        rapidjson::Value & result,
                        rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  rapidjson::Value vtypes(rapidjson::kObjectType);
  rapidjson::Value vtypes_str(rapidjson::kObjectType);
  rapidjson::Value a(rapidjson::kArrayType);
  rapidjson::Value val(rapidjson::kArrayType);
  rapidjson::Value src, dst;
  rapidjson::Value a_str(rapidjson::kArrayType);
  rapidjson::Value src_str, dst_str, etype, vtype, vtx_name;

  for (int64_t k = 0; k < nvlist; ++k) {
    const int64_t v = vlist[k];
    if(get_vtypes) {
      char intstr[21];
      sprintf(intstr, "%ld", (long)v);
      vtype.SetInt64(stinger_vtype_get(S, v));
      vtypes.AddMember(intstr, vtype, allocator);
      if(strings) {
        char * name = NULL;
        uint64_t len = 0;
        stinger_mapping_physid_direct(S, v, &name, &len);
        char * vtype_name = stinger_vtype_names_lookup_name(S,stinger_vtype_get(S, v));
        vtype.SetString(vtype_name, strlen(vtype_name), allocator);
        vtx_name.SetString(name, len, allocator);
        vtypes_str.AddMember(vtx_name, vtype, allocator);
      }
    }

    STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, v) {
      if (mark[STINGER_EDGE_DEST] > mark[STINGER_EDGE_SOURCE]) {
        src.SetInt64(STINGER_EDGE_SOURCE);
        dst.SetInt64(STINGER_EDGE_DEST);
        val.SetArray();
        val.PushBack(src, allocator);
        val.PushBack(dst, allocator);
        if(get_etypes) {
          etype.SetInt64(STINGER_EDGE_TYPE);
          val.PushBack(etype, allocator);
        }
        a.PushBack(val, allocator);
        if (strings) {
          char * physID;
          uint64_t len;
          if(-1 == stinger_mapping_physid_direct(S, STINGER_EDGE_SOURCE, &physID, &len)) {
            src_str.SetString("", 0, allocator);
          } else {
            src_str.SetString(physID, len, allocator);
          }
          if(-1 == stinger_mapping_physid_direct(S, STINGER_EDGE_DEST, &physID, &len)) {
            dst_str.SetString("", 0, allocator);
          } else {
            dst_str.SetString(physID, len, allocator);
          }
          val.SetArray();
          val.PushBack(src_str, allocator);
          val.PushBack(dst_str, allocator);
          if(get_etypes) {
            char * etype_str = stinger_etype_names_lookup_name(S, STINGER_EDGE_TYPE);
            etype.SetString(etype_str, strlen(etype_str), allocator);
            val.PushBack(etype, allocator);
          }
          a_str.PushBack(val, allocator);
        }
      }
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  result.AddMember("subgraph", a, allocator);
  if (strings)
    result.AddMember("subgraph_str", a_str, allocator);

  if(get_vtypes) {
    result.AddMember("vtypes", vtypes, allocator);
    if(strings) {
      result.AddMember("vtypes_str", vtypes_str, allocator);
    }
  }
}

int64_t 
JSON_RPC_label_breadth_first_search::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  params_array_t source;
  char * algorithm_name;
  char * data_array_name;
  int64_t limit;
  bool strings;
  bool get_types;
  bool get_etypes;
  bool get_vtypes; 

  rpc_params_t p[] = {
    {"source", TYPE_ARRAY, &source, false, 0},
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {"data", TYPE_STRING, &data_array_name, false, 0},
    {"limit", TYPE_INT64, &limit, true, INT64_MAX},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {"get_types", TYPE_BOOL, &get_types, true, 0},
    {"get_etypes", TYPE_BOOL, &get_etypes, true, 0},
    {"get_vtypes", TYPE_BOOL, &get_vtypes, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (!contains_params(p, params)) {
    return json_rpc_error(-32602, result, allocator);
  }

  if(get_types) {
    get_etypes = true;
    get_vtypes = true;
  }

  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return json_rpc_error(-32603, result, allocator);
  }

  /* vertex has no edges -- this is easy */
  if (source.len == 1 && stinger_outdegree (S, source.arr[0]) == 0) {
    rapidjson::Value a(rapidjson::kArrayType);
    result.AddMember("subgraph", a, allocator);
    return 0;
  }


  const int64_t NV = S->max_nv;
  int64_t nvlist = 0;
  int64_t * vlist;
  int64_t * mark;

  if (limit > NV) limit = NV;
  vlist = (int64_t*)xmalloc (limit * sizeof (*vlist));
  mark = (int64_t*)xcalloc (NV, sizeof (*mark));

  AlgDataArray labels(server_state, algorithm_name, data_array_name);

  const int64_t *label = labels.getptr<int64_t>();
  if (!label) return 0; /* XXX: Be more informative... */

  stinger_extract_bfs (S, source.len, source.arr, label, limit, -1, &nvlist, vlist, mark);

  // fprintf (stderr, "AUGH %ld\n", (long)nvlist);

  vlist_to_json_subgraph (S, nvlist, vlist, mark,
                          strings, get_types, get_etypes, get_vtypes,
                          result, allocator);

  free (mark);
  free (vlist);

  return 0;
}

int64_t 
JSON_RPC_label_mod_expand::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  params_array_t source;
  char * algorithm_name;
  char * data_array_name;
  int64_t limit;
  bool strings;
  bool get_types;
  bool get_etypes;
  bool get_vtypes; 

  rpc_params_t p[] = {
    {"source", TYPE_ARRAY, &source, false, 0},
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {"data", TYPE_STRING, &data_array_name, false, 0},
    {"limit", TYPE_INT64, &limit, true, INT64_MAX},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {"get_types", TYPE_BOOL, &get_types, true, 0},
    {"get_etypes", TYPE_BOOL, &get_etypes, true, 0},
    {"get_vtypes", TYPE_BOOL, &get_vtypes, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (!contains_params(p, params)) {
    return json_rpc_error(-32602, result, allocator);
  }

  if(get_types) {
    get_etypes = true;
    get_vtypes = true;
  }

  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return json_rpc_error(-32603, result, allocator);
  }

  /* vertex has no edges -- this is easy */
  if (source.len == 1 && stinger_outdegree (S, source.arr[0]) == 0) {
    rapidjson::Value a(rapidjson::kArrayType);
    result.AddMember("subgraph", a, allocator);
    return 0;
  }


  const int64_t NV = S->max_nv;
  int64_t nvlist = 0;
  int64_t * vlist;
  int64_t * mark;

  if (limit > NV) limit = NV;
  vlist = (int64_t*)xmalloc (limit * sizeof (*vlist));
  mark = (int64_t*)xcalloc (NV, sizeof (*mark));

  AlgDataArray labels(server_state, algorithm_name, data_array_name);

  const int64_t *label = labels.getptr<int64_t>();
  if (!label) return 0; /* XXX: Be more informative... */

  stinger_extract_mod (S, source.len, source.arr, label, limit, &nvlist, vlist, mark);

  vlist_to_json_subgraph (S, nvlist, vlist, mark,
                          strings, get_types, get_etypes, get_vtypes,
                          result, allocator);

  free (mark);
  free (vlist);

  return 0;
}


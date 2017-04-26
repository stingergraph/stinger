#include <queue>
#include <set>

//#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"
#include "json_rpc_server.h"
#include "json_rpc.h"

using namespace gt::stinger;


int64_t 
JSON_RPC_bfs_edges::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  int64_t source;
  bool strings;
  int64_t etype;

  rpc_params_t p[] = {
    {"source", TYPE_VERTEX, &source, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {"etype", TYPE_EDGE_TYPE , &etype, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (!contains_params(p, params)) {
    return json_rpc_error(-32602, result, allocator);
  }

  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return json_rpc_error(-32603, result, allocator);
  }

  rapidjson::Value a(rapidjson::kArrayType);
  rapidjson::Value a_str(rapidjson::kArrayType);
  rapidjson::Value val(rapidjson::kArrayType);
  rapidjson::Value src, dst;
  rapidjson::Value src_str, dst_str;

  /* vertex has no edges -- this is easy */
  if (stinger_outdegree (S, source) == 0) {
    result.AddMember("subgraph", a, allocator);
    return 0;
  }

  /* breadth-first search */
  int64_t nv = S->max_nv;

  int64_t * found   = (int64_t *) xmalloc (nv * sizeof(int64_t));
  for (int64_t i = 0; i < nv; i++) {
    found[i] = 0;
  }
  found[source] = 1;

  std::vector<std::set<int64_t> > levels;
  std::set<int64_t> frontier;

  frontier.insert(source);
  levels.push_back(frontier);

  std::set<int64_t>::iterator it;

  while (!levels.back().empty()) {
    frontier.clear();
    std::set<int64_t>& cur = levels.back();

    for (it = cur.begin(); it != cur.end(); it++) {
      int64_t v = *it;

      STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_BEGIN (S, etype, v) {
        int64_t k = STINGER_EDGE_DEST;
        if (!found[k]) {
          frontier.insert(k);
          found[k] = 1;

          /* the JSON */
          src.SetInt64(v);
          dst.SetInt64(k);
          val.SetArray();
          val.PushBack(src, allocator);
          val.PushBack(dst, allocator);
          a.PushBack(val, allocator);
          if (strings) {
            char * physID;
            uint64_t len;
            if(-1 == stinger_mapping_physid_direct(S, v, &physID, &len)) {
              src_str.SetString("", 0, allocator);
            } else {
              src_str.SetString(physID, len, allocator);
            }
            if(-1 == stinger_mapping_physid_direct(S, k, &physID, &len)) {
              dst_str.SetString("", 0, allocator);
            } else {
              dst_str.SetString(physID, len, allocator);
            }
            val.SetArray();
            val.PushBack(src_str, allocator);
            val.PushBack(dst_str, allocator);
            a_str.PushBack(val, allocator);
          }
        }
      } STINGER_FORALL_OUT_EDGES_OF_TYPE_OF_VTX_END();

    }

    levels.push_back(frontier);
  }


  result.AddMember("subgraph", a, allocator);
  if (strings) {
    result.AddMember("subgraph_str", a_str, allocator);
  }

  free (found);

  return 0;
}


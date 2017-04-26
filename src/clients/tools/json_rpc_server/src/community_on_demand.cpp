#include "json_rpc_server.h"
#include "json_rpc.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"
extern "C" {
  #include "stinger_alg/community_on_demand.h"
}

using namespace gt::stinger;

int64_t 
JSON_RPC_community_on_demand::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  bool strings;

  rpc_params_t p[] = {
    {"strings", TYPE_BOOL, &strings, true, 0},
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

  rapidjson::Value src, src_str;
  rapidjson::Value name, vtx_phys, value;
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_str (rapidjson::kArrayType);
  rapidjson::Value vtx_val (rapidjson::kArrayType);

  int64_t * vertices = NULL;
  int64_t * partitions = NULL;

  int64_t num_vertices = community_on_demand(S, &vertices, &partitions);

  for (int64_t i = 0; i < num_vertices; i++) {
    name.SetInt64(vertices[i]);
    vtx_id.PushBack(name, allocator);
    if (strings) {
      char * physID;
      uint64_t len;
      if(-1 == stinger_mapping_physid_direct(S, vertices[i], &physID, &len)) {
        physID = (char *) "";
        len = 0;
      }
      vtx_phys.SetString(physID, len, allocator);
      vtx_str.PushBack(vtx_phys, allocator);
    }
    value.SetInt64(partitions[i]);
    vtx_val.PushBack(value, allocator);
  }

  result.AddMember("vertex_id", vtx_id, allocator);
  if (strings)
    result.AddMember("vertex_str", vtx_str, allocator);
  result.AddMember("value", vtx_val, allocator);

  if (vertices != NULL) {
    xfree(vertices);
  }

  if (partitions != NULL) {
    xfree(partitions);
  }

  return 0;
}

#include "json_rpc_server.h"
#include "json_rpc.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"
extern "C" {
  #include "stinger_alg/adamic_adar.h"
}

using namespace gt::stinger;

int64_t 
JSON_RPC_adamic_adar::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  int64_t source;
  bool strings;
  int64_t etype;

  rpc_params_t p[] = {
    {"source", TYPE_VERTEX, &source, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {"etype", TYPE_EDGE_TYPE , &etype, true, -1},
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

  /* this part of the response needs to be set no matter what */
  src.SetInt64(source);
  result.AddMember("source", src, allocator);
  if (strings) {
    char * physID;
    uint64_t len;
    if(-1 == stinger_mapping_physid_direct(S, source, &physID, &len)) {
      physID = (char *) "";
      len = 0;
    }
    src_str.SetString(physID, len, allocator);
    result.AddMember("source_str", src_str, allocator);
  }

  int64_t * candidates = NULL;
  double * scores = NULL;

  int64_t num_candidates = adamic_adar(S, source, etype, &candidates, &scores);

  for (int64_t i = 0; i < num_candidates; i++) {
    name.SetInt64(candidates[i]);
    vtx_id.PushBack(name, allocator);
    if (strings) {
      char * physID;
      uint64_t len;
      if(-1 == stinger_mapping_physid_direct(S, candidates[i], &physID, &len)) {
        physID = (char *) "";
        len = 0;
      }
      vtx_phys.SetString(physID, len, allocator);
      vtx_str.PushBack(vtx_phys, allocator);
    }
    value.SetDouble(scores[i]);
    vtx_val.PushBack(value, allocator);
  }

  result.AddMember("vertex_id", vtx_id, allocator);
  if (strings)
    result.AddMember("vertex_str", vtx_str, allocator);
  result.AddMember("value", vtx_val, allocator);

  if (candidates != NULL) {
    xfree(candidates);
  }

  if (scores != NULL) {
    xfree(scores);
  }

  return 0;
}

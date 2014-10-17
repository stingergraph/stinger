//#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"
#include "json_rpc_server.h"
#include "json_rpc.h"

using namespace gt::stinger;


static int
compare (const void * a, const void * b)
{
    return ( *(int*)a - *(int*)b );
}

int64_t 
JSON_RPC_adamic_adar::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  int64_t source;
  int64_t target;
  bool strings;
  bool get_types;
  bool get_etypes;
  bool get_vtypes;

  rpc_params_t p[] = {
    {"source", TYPE_VERTEX, &source, false, 0},
    {"target", TYPE_VERTEX, &target, false, 0},
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

  rapidjson::Value src, src_str, src_vtype, src_vtype_str;
  rapidjson::Value dst, dst_str, dst_vtype, dst_vtype_str;
  rapidjson::Value score;


  int64_t source_deg = stinger_outdegree (S, source);
  int64_t target_deg = stinger_outdegree (S, target);

  /* vertex has no edges -- this is easy */
  if (source_deg == 0 || target_deg == 0) {
    src.SetInt64(source);
    result.AddMember("source", src, allocator);

    dst.SetInt64(target);
    result.AddMember("target", dst, allocator);

    score.SetDouble(0.0);
    result.AddMember("value", score, allocator);
    return 0;
  }

  int64_t * source_adj = (int64_t *) xmalloc (source_deg * sizeof(int64_t));
  int64_t * target_adj = (int64_t *) xmalloc (target_deg * sizeof(int64_t));

  /* extract and sort the neighborhood of SOURCE */
  size_t res;
  stinger_gather_successors (S, source, &res, source_adj, NULL, NULL, NULL, NULL, source_deg);
  qsort (source_adj, source_deg, sizeof(int64_t), compare);

  /* extract and sort the neighborhood of TARGET */
  stinger_gather_successors (S, target, &res, target_adj, NULL, NULL, NULL, NULL, target_deg);
  qsort (target_adj, target_deg, sizeof(int64_t), compare);

  double adamic_adar_score = 0.0;

  int64_t i = 0, j = 0;
  while (i < source_deg && j < target_deg)
  {
    if (source_adj[i] < target_adj[j]) {
      i++;
    }
    else if (target_adj[j] < source_adj[i]) {
      j++;
    }
    else /* if source_adj[i] == target_adj[j] */
    {
      int64_t neighbor = source_adj[i];
      int64_t my_deg = stinger_outdegree(S, neighbor);
      double score = 1.0 / log ((double) my_deg);
      adamic_adar_score += score;
      i++;
      j++;
    }
  }

  free (source_adj);
  free (target_adj);
 
  /* configure the response */

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

  dst.SetInt64(target);
  result.AddMember("target", dst, allocator);
  if (strings) {
    char * physID;
    uint64_t len;
    if(-1 == stinger_mapping_physid_direct(S, target, &physID, &len)) {
      physID = (char *) "";
      len = 0;
    }
    dst_str.SetString(physID, len, allocator);
    result.AddMember("target_str", dst_str, allocator);
  }

  score.SetDouble(adamic_adar_score);
  result.AddMember("value", score, allocator);

  return 0;
}

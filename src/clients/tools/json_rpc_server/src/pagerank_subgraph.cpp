#include <queue>
#include <set>

//#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"
#include "json_rpc_server.h"
#include "json_rpc.h"
extern "C" {
  #include "stinger_alg/pagerank.h"
}

using namespace gt::stinger;


int64_t 
JSON_RPC_pagerank_subgraph::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  params_array_t vertices;
  bool strings;

  rpc_params_t p[] = {
    {"vertices", TYPE_ARRAY, &vertices, false, 0},
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

  uint8_t * vtx_subset = (uint8_t *)xcalloc(S->max_nv,sizeof(uint8_t));

  for (int64_t i = 0; i < vertices.len; i++) {
    vtx_subset[vertices.arr[i]] = 1;
  }

  double * pr = (double *)xcalloc(S->max_nv,sizeof(double));
  OMP("omp parallel for")
  for(int64_t i = 0; i < vertices.len; i++) {
    pr[vertices.arr[i]] = 1 / ((double)vertices.len);
  }

  double * tmp_pr = (double *)xcalloc(S->max_nv, sizeof(double));

  double epsilon = EPSILON_DEFAULT;
  double dampingfactor = DAMPINGFACTOR_DEFAULT;
  int64_t maxiter = MAXITER_DEFAULT;

  page_rank_subset((stinger_t *)S, (int64_t)S->max_nv, (uint8_t *)vtx_subset, (int64_t)vertices.len, 
                    (double *)pr, (double *)tmp_pr, 
                    (double)epsilon, (double)dampingfactor, (int64_t)maxiter);

  xfree(tmp_pr);
  xfree(vtx_subset);

  // CREATE the JSON
  rapidjson::Value vertex_id_json(rapidjson::kArrayType);
  rapidjson::Value value_json(rapidjson::kArrayType);
  rapidjson::Value pagerank_json (rapidjson::kObjectType);
  rapidjson::Value vtx_str (rapidjson::kArrayType);
  rapidjson::Value vtx_phys;

  for (int64_t i = 0; i < vertices.len; i++) {
    uint64_t vtx = vertices.arr[i];
    vertex_id_json.PushBack(vtx,allocator);
    value_json.PushBack(pr[vtx],allocator);
    if (strings) {
      char * physID;
      uint64_t len;
      if(-1 == stinger_mapping_physid_direct(S, vtx, &physID, &len)) {
        physID = (char *) "";
        len = 0;
      }
      vtx_phys.SetString(physID, len, allocator);
      vtx_str.PushBack(vtx_phys, allocator);
    }
  }
  
  pagerank_json.AddMember("vertex_id", vertex_id_json, allocator);
  pagerank_json.AddMember("value", value_json, allocator);
  if (strings)
      pagerank_json.AddMember("vertex_str", vtx_str, allocator);

  result.AddMember("pagerank",pagerank_json,allocator);

  xfree(pr);
  return 0;
}


//#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"
#include "json_rpc_server.h"
#include "json_rpc.h"

using namespace gt::stinger;


int64_t 
JSON_RPC_conversation_id_by_user::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  int64_t source;
  bool strings;

  rpc_params_t p[] = {
    {"source", TYPE_VERTEX, &source, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (!contains_params(p, params)) {
    return json_rpc_error(-32602, result, allocator);
  }
  
  StingerAlgState * alg_state = server_state->get_alg("influence_aggregator");
  if (!alg_state) {
    LOG_E ("Algorithm is not running");
    return json_rpc_error(-32003, result, allocator);
  }

  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return json_rpc_error(-32603, result, allocator);
  }
 
  /* read-only data arrays */
  int64_t * component_map = (int64_t *)alg_state->data;
  int64_t * component_size  = component_map + S->max_nv;

  rapidjson::Value src, src_str;
  rapidjson::Value name, vtx_phys;
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_str (rapidjson::kArrayType);


  /* this part of the response needs to be set no matter what */
  src.SetInt64(source);
  result.AddMember("source", src, allocator);
  if (strings) {
    char * physID;
    uint64_t len;
    if (-1 == stinger_mapping_physid_direct(S, source, &physID, &len)) {
      physID = (char *) "";
      len = 0;
    }
    src_str.SetString(physID, len, allocator);
    result.AddMember("source_str", src_str, allocator);
  }

  int64_t tweets_etype = stinger_etype_names_lookup_type(S, "tweets");
  if (tweets_etype < 0) {
    return -1;
  }

  /* this is a pretty expensive way of unique-ing the conversation IDs */
  /* maybe consider doing something different (i.e. sorting) */
  int64_t nv = S->max_nv;
  int64_t * conv_ids = (int64_t *) xmalloc (nv * sizeof(int64_t));
  for (int64_t i = 0; i < nv; i++) {
    conv_ids[i] = 0;
  }

  /* find unique conversation IDs */
  STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_BEGIN(S, tweets_etype, source) {
    int64_t tweet_id = STINGER_EDGE_DEST;
    int64_t conversation_id = component_map[tweet_id];
    conv_ids[conversation_id] = 1;
  } STINGER_FORALL_EDGES_OF_TYPE_OF_VTX_END();

  for (int64_t i = 0; i < nv; i++) {
    if (conv_ids[i]) {
      name.SetInt64(i);
      vtx_id.PushBack(name, allocator);
      if (strings) {
	char * physID;
	uint64_t len;
	if (-1 == stinger_mapping_physid_direct(S, i, &physID, &len)) {
	  physID = (char *) "";
	  len = 0;
	}
	vtx_phys.SetString(physID, len, allocator);
	vtx_str.PushBack(vtx_phys, allocator);
      }
    }
  }
 
  /* configure the response */
  result.AddMember("vertex_id", vtx_id, allocator);
  if (strings) {
    result.AddMember("vertex_str", vtx_str, allocator);
  }

  free (conv_ids);

  return 0;
}

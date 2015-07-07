//#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"
#include "json_rpc_server.h"
#include "json_rpc.h"

using namespace gt::stinger;


int64_t 
JSON_RPC_get_connected_component::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  int64_t source;
  bool show_edges;
  bool show_vertices;
  bool strings;

  rpc_params_t p[] = {
    {"source", TYPE_VERTEX, &source, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {"show_vertices", TYPE_BOOL, &show_vertices, true, 1},
    {"show_edges", TYPE_BOOL, &show_edges, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (!contains_params(p, params)) {
    return json_rpc_error(-32602, result, allocator);
  }
  
  StingerAlgState * alg_state = server_state->get_alg("weakly_connected_components");
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
  rapidjson::Value component_id, component_id_str;
  rapidjson::Value name, vtx_phys;
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_str (rapidjson::kArrayType);
  rapidjson::Value edge_id (rapidjson::kArrayType);
  rapidjson::Value edge_str (rapidjson::kArrayType);


  /* translate the source vertex */
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

  int64_t this_component_id = component_map[source];

  /* set the component ID for the vertex in question */
  component_id.SetInt64(this_component_id);
  result.AddMember("component", component_id, allocator);
  if (strings) {
    char * physID;
    uint64_t len;
    if (-1 == stinger_mapping_physid_direct(S, this_component_id, &physID, &len)) {
      physID = (char *) "";
      len = 0;
    }
    component_id_str.SetString(physID, len, allocator);
    result.AddMember("component_str", component_id_str, allocator);
  }

  /* add the vertices that are in the connected component */
  int64_t nv = S->max_nv;
  if (show_vertices) {
    for (int64_t i = 0; i < nv; i++) {
      if (component_map[i] == this_component_id) {
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
  }

  /* add the edges in the connected component */
  if (show_edges) {
    /* TODO: not implemented yet */


    /* configure the response */
    result.AddMember("edge_id", edge_id, allocator);
    if (strings) {
      result.AddMember("edge_str", edge_str, allocator);
    }
  }

  return 0;
}

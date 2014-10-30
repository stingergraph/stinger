//#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

#include "rapidjson/document.h"
#include "json_rpc_server.h"
#include "json_rpc.h"

using namespace gt::stinger;


int64_t 
JSON_RPC_get_graph_stats::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  bool get_types;
  rpc_params_t p[] = {
    {"get_types", TYPE_BOOL, &get_types, true, 0},
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

  rapidjson::Value nv;
  rapidjson::Value ne;
  rapidjson::Value max_time_seen;
  rapidjson::Value edge_types;
  rapidjson::Value vertex_types;

  /* default: counts of types, otherwise: lists of type strings */
  int64_t num_edge_types = stinger_etype_names_count(S);
  int64_t num_vertex_types = stinger_vtype_names_count(S);
  if(!get_types) {
    edge_types.SetInt64(num_edge_types);
    vertex_types.SetInt64(num_vertex_types);
  } else {
    rapidjson::Value type_name;

    edge_types.SetArray();
    for(int64_t i = 0; i < num_edge_types; i++) {
      char * type_name_str = stinger_etype_names_lookup_name(S,i);
      type_name.SetString(type_name_str,strlen(type_name_str),allocator);
      edge_types.PushBack(type_name, allocator);
    }

    vertex_types.SetArray();
    for(int64_t i = 0; i < num_vertex_types; i++) {
      char * type_name_str = stinger_vtype_names_lookup_name(S,i);
      type_name.SetString(type_name_str,strlen(type_name_str),allocator);
      vertex_types.PushBack(type_name, allocator);
    }
  }

  int64_t num_vertices = stinger_mapping_nv(S);
  nv.SetInt64(num_vertices);
  int64_t num_edges = stinger_total_edges(S);
  ne.SetInt64(num_edges);
  
  max_time_seen.SetInt64(server_state->get_max_time());

  result.AddMember("vertices", nv, allocator);
  result.AddMember("edges", ne, allocator);
  result.AddMember("time", max_time_seen, allocator);
  result.AddMember("edge_types", edge_types, allocator);
  result.AddMember("vertex_types", vertex_types, allocator);

  return 0;
}


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
JSON_RPC_egonet::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  int64_t source;
  bool strings;
  bool get_types;
  bool get_etypes;
  bool get_vtypes;
  bool incident_edges;

  rpc_params_t p[] = {
    {"source", TYPE_VERTEX, &source, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {"get_types", TYPE_BOOL, &get_types, true, 0},
    {"get_etypes", TYPE_BOOL, &get_etypes, true, 0},
    {"get_vtypes", TYPE_BOOL, &get_vtypes, true, 0},
    {"incident_edges", TYPE_BOOL, &incident_edges, true, 0},
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

  rapidjson::Value edges(rapidjson::kArrayType);
  rapidjson::Value edges_str(rapidjson::kArrayType);
  rapidjson::Value vtx(rapidjson::kArrayType);
  rapidjson::Value vtx_str(rapidjson::kArrayType);
  rapidjson::Value center, center_str;
  rapidjson::Value src, dst;
  rapidjson::Value src_str, dst_str;
  rapidjson::Value val(rapidjson::kArrayType);
  
  rapidjson::Value vtypes(rapidjson::kObjectType);
  rapidjson::Value vtypes_str(rapidjson::kObjectType);
  rapidjson::Value etype, vtype;

  /* vertex has no edges -- this is easy */
  if (stinger_outdegree (S, source) == 0) {
    result.AddMember("egonet", edges, allocator);
    return 0;
  }
 
  /* this array will hold the neighbor set */
  int64_t nv = S->max_nv;
  int64_t * marks = (int64_t *) xmalloc (nv * sizeof(int64_t));
  for (int64_t i = 0; i < nv; i++) {
    marks[i] = 0;
  }

  /* mark and insert source if we are returning incident edges */
  if (incident_edges) {
    marks[source] = 1;
    center.SetInt64(source);
    vtx.PushBack(center, allocator);
    
    if (strings) {
      char * physID;
      uint64_t len;
      if (-1 == stinger_mapping_physid_direct(S, source, &physID, &len)) {
	physID = (char *) "";
	len = 0;
      }
      center_str.SetString(physID, len, allocator);
      vtx_str.PushBack(center_str, allocator);
    }
  }

  /* mark all neighbors of the source vertex */
  STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, source) {
    int64_t u = STINGER_EDGE_DEST;
    marks[u] = 1;
    src.SetInt64(u);
    vtx.PushBack(src, allocator);

    if (strings) {
      char * physID;
      uint64_t len;
      if (-1 == stinger_mapping_physid_direct(S, u, &physID, &len)) {
	physID = (char *) "";
	len = 0;
      }
      src_str.SetString(physID, len, allocator);
      vtx_str.PushBack(src_str, allocator);
    }

    if (incident_edges) {
      src.SetInt64(source);
      dst.SetInt64(u);
      val.SetArray();
      val.PushBack(src, allocator);
      val.PushBack(dst, allocator);
      edges.PushBack(val, allocator);

      if (strings) {
	char * physID;
	uint64_t len;
	if (-1 == stinger_mapping_physid_direct(S, source, &physID, &len)) {
	  physID = (char *) "";
	  len = 0;
	}
	src_str.SetString(physID, len, allocator);
	
	if (-1 == stinger_mapping_physid_direct(S, u, &physID, &len)) {
	  physID = (char *) "";
	  len = 0;
	}
	dst_str.SetString(physID, len, allocator);

	val.SetArray();
	val.PushBack(src_str, allocator);
	val.PushBack(dst_str, allocator);
	edges_str.PushBack(val, allocator);
      }
    }

  } STINGER_FORALL_EDGES_OF_VTX_END();

  /* now we will find all of the edges in common between the neighbors */
  STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, source) {
    int64_t u = STINGER_EDGE_DEST;

    STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, u) {
      int64_t v = STINGER_EDGE_DEST;

      if (marks[v]) {
	src.SetInt64(u);
	dst.SetInt64(v);
	val.SetArray();
	val.PushBack(src, allocator);
	val.PushBack(dst, allocator);
	edges.PushBack(val, allocator);
	
	if (strings) {
	  char * physID;
	  uint64_t len;
	  if (-1 == stinger_mapping_physid_direct(S, u, &physID, &len)) {
	    physID = (char *) "";
	    len = 0;
	  }
	  src_str.SetString(physID, len, allocator);

	  if (-1 == stinger_mapping_physid_direct(S, v, &physID, &len)) {
	    physID = (char *) "";
	    len = 0;
	  }
	  dst_str.SetString(physID, len, allocator);
	  val.SetArray();
	  val.PushBack(src_str, allocator);
	  val.PushBack(dst_str, allocator);
	  edges_str.PushBack(val, allocator);
	}
      }

    } STINGER_FORALL_EDGES_OF_VTX_END();
  } STINGER_FORALL_EDGES_OF_VTX_END();


  result.AddMember("vertices", vtx, allocator);
  if (strings) {
    result.AddMember("vertices_str", vtx_str, allocator);
  }

  result.AddMember("subgraph", edges, allocator);
  if (strings) {
    result.AddMember("subgraph_str", edges_str, allocator);
  }

  if (get_vtypes) {
    result.AddMember("vtypes", vtypes, allocator);
    if (strings) {
      result.AddMember("vtypes_str", vtypes_str, allocator);
    }
  }

  free(marks);

  return 0;
}


#include "json_rpc_server.h"
#include "json_rpc.h"
#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"
#include "time.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

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

  /* find a vertex with more than 10 edges and less than 50 */
  int64_t max_nv = S->max_nv;
  int64_t selected_vertex = -1;
  srand(time(NULL));
  int64_t start = rand() % max_nv;
  for (int64_t i = start; i < max_nv; i++) {
    int64_t outdegree = stinger_outdegree(S, i);
    if (outdegree < 50 && outdegree > 10) {
      selected_vertex = i;
      break;
    }
  }
  if (selected_vertex == -1) {
    for (int64_t i = 0; i < max_nv; i++) {
      int64_t outdegree = stinger_outdegree(S, i);
      if (outdegree < 50 && outdegree > 10) {
	selected_vertex = i;
	break;
      }
    }
  }
  if (selected_vertex == -1) {
    selected_vertex = 7;
  }
  source = selected_vertex;


  /* array to hold a single edge */
  rapidjson::Value val(rapidjson::kArrayType);

  /* array to hold all edges (virtually addressed) */
  rapidjson::Value edges(rapidjson::kArrayType);

  /* array to hold all edges (physically addressed) */
  rapidjson::Value edges_str(rapidjson::kArrayType);

  /* array to hold all vertices (virtually addressed) */
  rapidjson::Value vtx(rapidjson::kArrayType);

  /* array to hold all vertices (physically addressed) */
  rapidjson::Value vtx_str(rapidjson::kArrayType);

  /* values to hold vertex information */
  rapidjson::Value source_v, source_v_str;
  rapidjson::Value vtype, vtype_str;
  rapidjson::Value center, center_str;
  rapidjson::Value src, dst;
  rapidjson::Value src_str, dst_str;
  rapidjson::Value etype, etype_str;

  /* array to hold vertex types */
  rapidjson::Value vtypes(rapidjson::kArrayType);

  /* array to hold vertex type strings */
  rapidjson::Value vtypes_str(rapidjson::kArrayType);

  /* source info */
  source_v.SetInt64(source);
  result.AddMember("source", source_v, allocator);
  if (strings) {
    char * physID;
    uint64_t len;
    if (-1 == stinger_mapping_physid_direct(S, source, &physID, &len)) {
      physID = (char *) "";
      len = 0;
    }
    source_v_str.SetString(physID, len, allocator);
    result.AddMember("source_str", source_v_str, allocator);
  }

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

    if (get_vtypes) {
      int64_t source_type = stinger_vtype_get(S, source);
      vtype.SetInt64(source_type);
      vtypes.PushBack(vtype, allocator);

      if (strings) {
	char * name = NULL;
	uint64_t len = 0;
	char * vtype_name = stinger_vtype_names_lookup_name(S, source_type);
	vtype_str.SetString(vtype_name, strlen(vtype_name), allocator);
	vtypes_str.PushBack(vtype_str, allocator);
      }
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
      if (get_etypes) {
	etype.SetInt64(STINGER_EDGE_TYPE);
	val.PushBack(etype, allocator);
      }
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
	if (get_etypes) {
	  char * etype_str_ptr = stinger_etype_names_lookup_name(S, STINGER_EDGE_TYPE);
	  etype_str.SetString(etype_str_ptr, strlen(etype_str_ptr), allocator);
	  val.PushBack(etype_str, allocator);
	}
	edges_str.PushBack(val, allocator);
      }
    }
    
    if (get_vtypes) {
      int64_t source_type = stinger_vtype_get(S, u);
      vtype.SetInt64(source_type);
      vtypes.PushBack(vtype, allocator);

      if (strings) {
	char * name = NULL;
	uint64_t len = 0;
	char * vtype_name = stinger_vtype_names_lookup_name(S, source_type);
	vtype_str.SetString(vtype_name, strlen(vtype_name), allocator);
	vtypes_str.PushBack(vtype_str, allocator);
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
	if (get_etypes) {
	  etype.SetInt64(STINGER_EDGE_TYPE);
	  val.PushBack(etype, allocator);
	}
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
	  if (get_etypes) {
	    char * etype_str_ptr = stinger_etype_names_lookup_name(S, STINGER_EDGE_TYPE);
	    etype_str.SetString(etype_str_ptr, strlen(etype_str_ptr), allocator);
	    val.PushBack(etype_str, allocator);
	  }
	  edges_str.PushBack(val, allocator);
	}
      }

    } STINGER_FORALL_EDGES_OF_VTX_END();
  } STINGER_FORALL_EDGES_OF_VTX_END();


  result.AddMember("vertices", vtx, allocator);
  if (strings) {
    result.AddMember("vertices_str", vtx_str, allocator);
  }
  
  if (get_vtypes) {
    result.AddMember("vtypes", vtypes, allocator);
    if (strings) {
      result.AddMember("vtypes_str", vtypes_str, allocator);
    }
  }

  result.AddMember("egonet", edges, allocator);
  if (strings) {
    result.AddMember("egonet_str", edges_str, allocator);
  }


  free(marks);

  return 0;
}


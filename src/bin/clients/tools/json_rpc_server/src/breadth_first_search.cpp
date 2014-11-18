#include "json_rpc_server.h"
#include "json_rpc.h"
#include <queue>
#include <set>

#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


int64_t 
JSON_RPC_breadth_first_search::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
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

  rapidjson::Value vtypes(rapidjson::kObjectType);
  rapidjson::Value vtypes_str(rapidjson::kObjectType);
  rapidjson::Value a(rapidjson::kArrayType);
  rapidjson::Value val(rapidjson::kArrayType);
  rapidjson::Value src, dst;
  rapidjson::Value a_str(rapidjson::kArrayType);
  rapidjson::Value src_str, dst_str, etype, vtype, vtx_name;

  /* vertex has no edges -- this is easy */
  if (stinger_outdegree (S, source) == 0 || stinger_indegree (S, target) == 0) {
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

  if(get_vtypes) {
    char intstr[21];
    sprintf(intstr, "%ld", (long)target);
    vtype.SetInt64(stinger_vtype_get(S, target));
    vtypes.AddMember(intstr, vtype, allocator);
    if(strings) {
      char * name = NULL;
      uint64_t len = 0;
      stinger_mapping_physid_direct(S, target, &name, &len);
      char * vtype_name = stinger_vtype_names_lookup_name(S,stinger_vtype_get(S, target));
      vtype.SetString(vtype_name, strlen(vtype_name), allocator);
      vtx_name.SetString(name, len, allocator);
      vtypes_str.AddMember(vtx_name, vtype, allocator);
    }
  }

  std::vector<std::set<int64_t> > levels;
  std::set<int64_t> frontier;

  frontier.insert(source);
  LOG_D_A("pushing onto levels, size %ld", levels.size());
  levels.push_back(frontier);
  LOG_D_A("-------      levels, size %ld", levels.size());

  std::set<int64_t>::iterator it;

  while (!found[target] && !levels.back().empty()) {
    frontier.clear();
    std::set<int64_t>& cur = levels.back();

    for (it = cur.begin(); it != cur.end(); it++) {
      int64_t v = *it;

      STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, v) {
	if (!found[STINGER_EDGE_DEST]) {
	  frontier.insert(STINGER_EDGE_DEST);
	  found[STINGER_EDGE_DEST] = 1;
	}
      } STINGER_FORALL_EDGES_OF_VTX_END();

    }

    LOG_D_A("pushing onto levels, size %ld", levels.size());
    levels.push_back(frontier);
    LOG_D_A("-------      levels, size %ld", levels.size());
  }

  if (!found[target]) {
    result.AddMember("subgraph", a, allocator);
    return 0;
  }

  std::queue<int64_t> * Q = new std::queue<int64_t>();
  std::queue<int64_t> * Qnext = new std::queue<int64_t>();
  std::queue<int64_t> * tempQ;

  LOG_D_A("popping off of levels, size %ld", levels.size());
  levels.pop_back();  /* this was the level that contained the target */
  LOG_D_A("-------        levels, size %ld", levels.size());
  std::set<int64_t>& cur = levels.back();

  STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, target) {
    if (cur.find(STINGER_EDGE_DEST) != cur.end()) {
      Q->push(STINGER_EDGE_DEST); 
      /* return edge <target, STINGER_EDGE_DEST> */
      src.SetInt64(target);
      dst.SetInt64(STINGER_EDGE_DEST);
      val.SetArray();
      val.PushBack(src, allocator);
      val.PushBack(dst, allocator);
      if(get_etypes) {
	etype.SetInt64(STINGER_EDGE_TYPE);
	val.PushBack(etype, allocator);
      }
      if(get_vtypes) {
	char intstr[21];
	sprintf(intstr, "%ld", (long)STINGER_EDGE_DEST);
	vtype.SetInt64(stinger_vtype_get(S, STINGER_EDGE_DEST));
	vtypes.AddMember(intstr, vtype, allocator);
	if(strings) {
	  char * name = NULL;
	  uint64_t len = 0;
	  stinger_mapping_physid_direct(S, STINGER_EDGE_DEST, &name, &len);
	  char * vtype_name = stinger_vtype_names_lookup_name(S,stinger_vtype_get(S, STINGER_EDGE_DEST));
	  vtype.SetString(vtype_name, strlen(vtype_name), allocator);
	  vtx_name.SetString(name, len, allocator);
	  vtypes_str.AddMember(vtx_name, vtype, allocator);
	}
      }
      a.PushBack(val, allocator);
      if (strings) {
	char * physID;
	uint64_t len;
	if(-1 == stinger_mapping_physid_direct(S, target, &physID, &len)) {
	  src_str.SetString("", 0, allocator);
	} else {
	  src_str.SetString(physID, len, allocator);
	}
	if(-1 == stinger_mapping_physid_direct(S, STINGER_EDGE_DEST, &physID, &len)) {
	  dst_str.SetString("", 0, allocator);
	} else {
	  dst_str.SetString(physID, len, allocator);
	}
	val.SetArray();
	val.PushBack(src_str, allocator);
	val.PushBack(dst_str, allocator);
	if(get_etypes) {
	  char * etype_str = stinger_etype_names_lookup_name(S, STINGER_EDGE_TYPE);
	  etype.SetString(etype_str, strlen(etype_str), allocator);
	  val.PushBack(etype, allocator);
	}
	a_str.PushBack(val, allocator);
      }
    }
  } STINGER_FORALL_EDGES_OF_VTX_END();

  LOG_D_A("popping off of levels, size %ld", levels.size());
  levels.pop_back();
  LOG_D_A("-------        levels, size %ld", levels.size());

  while (!levels.empty()) {
    std::set<int64_t>& cur = levels.back();

    while (!Q->empty()) {
      int64_t v = Q->front();
      Q->pop();

      STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, v) {
	if (cur.find(STINGER_EDGE_DEST) != cur.end()) {
	  Qnext->push(STINGER_EDGE_DEST);
	  /* return edge <target, STINGER_EDGE_DEST> */
	  src.SetInt64(v);
	  dst.SetInt64(STINGER_EDGE_DEST);
	  val.SetArray();
	  val.PushBack(src, allocator);
	  val.PushBack(dst, allocator);
	  if(get_etypes) {
	    etype.SetInt64(STINGER_EDGE_TYPE);
	    val.PushBack(etype, allocator);
	  }
	  if(get_vtypes) {
	    char intstr[21];
	    sprintf(intstr, "%ld", (long)STINGER_EDGE_DEST);
	    vtype.SetInt64(stinger_vtype_get(S, STINGER_EDGE_DEST));
	    vtypes.AddMember(intstr, vtype, allocator);
	    if(strings) {
	      char * name = NULL;
	      uint64_t len = 0;
	      stinger_mapping_physid_direct(S, STINGER_EDGE_DEST, &name, &len);
	      char * vtype_name = stinger_vtype_names_lookup_name(S,stinger_vtype_get(S, STINGER_EDGE_DEST));
	      vtype.SetString(vtype_name, strlen(vtype_name), allocator);
	      vtx_name.SetString(name, len, allocator);
	      vtypes_str.AddMember(vtx_name, vtype, allocator);
	    }
	  }
	  a.PushBack(val, allocator);
	  if (strings) {
	    char * physID;
	    uint64_t len;
	    if(-1 == stinger_mapping_physid_direct(S, v, &physID, &len)) {
	      src_str.SetString("", 0, allocator);
	    } else {
	      src_str.SetString(physID, len, allocator);
	    }
	    if(-1 == stinger_mapping_physid_direct(S, STINGER_EDGE_DEST, &physID, &len)) {
	      dst_str.SetString("", 0, allocator);
	    } else {
	      dst_str.SetString(physID, len, allocator);
	    }
	    val.SetArray();
	    val.PushBack(src_str, allocator);
	    val.PushBack(dst_str, allocator);
	    if(get_etypes) {
	      char * etype_str = stinger_etype_names_lookup_name(S, STINGER_EDGE_TYPE);
	      etype.SetString(etype_str, strlen(etype_str), allocator);
	      val.PushBack(etype, allocator);
	    }
	    a_str.PushBack(val, allocator);
	  }
	}
      } STINGER_FORALL_EDGES_OF_VTX_END();
    }

    LOG_D_A("popping off of levels, size %ld", levels.size());
    levels.pop_back();
    LOG_D_A("-------        levels, size %ld", levels.size());

    tempQ = Q;
    Q = Qnext;
    Qnext = tempQ;

  }

  result.AddMember("subgraph", a, allocator);
  if (strings)
    result.AddMember("subgraph_str", a_str, allocator);

  if(get_vtypes) {
    result.AddMember("vtypes", vtypes, allocator);
    if(strings) {
      result.AddMember("vtypes_str", vtypes_str, allocator);
    }
  }

  delete Q;
  delete Qnext;
  free (found);

  return 0;
}


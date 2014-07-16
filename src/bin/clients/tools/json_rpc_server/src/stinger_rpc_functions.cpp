#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <functional>
#include <queue>
#include <set>
#include <vector>
#include <climits>
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807 /* sometimes c++ just doesn't want this to work */
#endif

//#define LOG_AT_W  /* warning only */

extern "C" {
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
}

#include "rapidjson/document.h"

#include "json_rpc_server.h"
#include "json_rpc.h"
#include "session_handling.h"

using namespace gt::stinger;


int64_t 
JSON_RPC_get_server_info::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  rpc_params_t p[] = {
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (!contains_params(p, params)) {
    return json_rpc_error(-32602, result, allocator);
  }

  pid_t myPid = getpid();

  uint64_t key = myPid;
  key = (~key) + (key << 21); // key = (key << 21) - key - 1;
  key = key ^ (key >> 24);
  key = (key + (key << 3)) + (key << 8); // key * 265
  key = key ^ (key >> 14);
  key = (key + (key << 2)) + (key << 4); // key * 21
  key = key ^ (key >> 28);
  key = key + (key << 31);

  rapidjson::Value pid;
  pid.SetInt64(key);
  result.AddMember("pid", pid, allocator);

  return 0;
}


int64_t 
JSON_RPC_register::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  char * type_name;
  rpc_params_t p[] = {
    {"type", TYPE_STRING, &type_name, false, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  LOG_D ("Checking for parameter \"type:\"");
  if (!contains_params(p, params)) {
    return json_rpc_error(-32602, result, allocator);
  }
  
  /* Does the session type exist */
  LOG_D_A ("Searching for session type: %s", type_name);
  if (server_state->has_rpc_session(type_name) == false) {
    return json_rpc_error (-32601, result, allocator);
  }

  LOG_D ("Get a session id and create a session");
  /* create a session id and a new session of the requested type */
  /* add the session to the server state */
  int64_t next_session_id = server_state->get_next_session();
  JSON_RPCSession * session = server_state->get_rpc_session(type_name)->gimme(next_session_id, server_state);

  rapidjson::Value session_id;
  session_id.SetInt64(next_session_id);
  result.AddMember("session_id", session_id, allocator);

  LOG_D ("Check parameters for the session type");

  rpc_params_t * session_params = session->get_params();
  /* if things don't go so well, we need to undo everything to this point */
  if (!contains_params(session_params, params)) {
    delete session;
    return json_rpc_error(-32602, result, allocator);
  }

  session->lock();  /* I wish I didn't have to do this */
  /* push the session onto the stack */
  int64_t rtn = server_state->add_session(next_session_id, session);
  if (rtn == -1) {
    delete session;
    return json_rpc_error(-32002, result, allocator);
  }

  LOG_D ("Call the onRegister method for the session");

  /* this will send back the edge list to the client */
  //session->lock();
  session->onRegister(result, allocator);
  session->unlock();

  LOG_D ("Return");

  return 0;
}


int64_t 
JSON_RPC_request::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  int64_t session_id;
  bool strings;
  rpc_params_t p[] = {
    {"session_id", TYPE_INT64, &session_id, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  LOG_D ("Checking for parameters");

  if (!contains_params(p, params)) {
    return json_rpc_error(-32602, result, allocator);
  }

  LOG_D_A ("Check if session id %ld is valid", session_id);
  JSON_RPCSession * session = server_state->get_session(session_id);

  if (!session) {
    return json_rpc_error(-32001, result, allocator);
  }

  rapidjson::Value json_session_id;
  json_session_id.SetInt64(session_id);
  result.AddMember("session_id", json_session_id, allocator);

  LOG_D ("Call the onRequest method for the session");

  /* this will send back the edge list to the client */
  session->lock();
  session->onRequest(result, allocator);
  session->reset_timeout();
  session->unlock();

  rapidjson::Value time_since;
  time_since.SetInt64(session->get_time_since());
  result.AddMember("time_since", time_since, allocator);

  LOG_D ("Return");

  return 0;
}


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
  int64_t num_edges = S->cur_ne; //stinger_edges_up_to(S, num_vertices);
  ne.SetInt64(num_edges);
  
  max_time_seen.SetInt64(server_state->get_max_time());

  result.AddMember("vertices", nv, allocator);
  result.AddMember("edges", ne, allocator);
  result.AddMember("time", max_time_seen, allocator);
  result.AddMember("edge_types", edge_types, allocator);
  result.AddMember("vertex_types", vertex_types, allocator);

  return 0;
}


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
  if (stinger_outdegree (S, source) == 0 || stinger_outdegree (S, target) == 0) {
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

int64_t 
JSON_RPC_label_breadth_first_search::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  int64_t source;
  char * algorithm_name;
  char * data_array_name;
  int64_t limit;
  bool strings;
  bool get_types;
  bool get_etypes;
  bool get_vtypes; 

  rpc_params_t p[] = {
    {"source", TYPE_VERTEX, &source, false, 0},
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {"data", TYPE_STRING, &data_array_name, false, 0},
    {"limit", TYPE_INT64, &limit, true, INT64_MAX},
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
  if (stinger_outdegree (S, source) == 0) {
    result.AddMember("subgraph", a, allocator);
    return 0;
  }

  AlgDataArray labels(server_state, algorithm_name, data_array_name);

  /* breadth-first search */
  int64_t nv = S->max_nv;

  int64_t * found = (int64_t *) xmalloc (nv * sizeof(int64_t));
  for (int64_t i = 0; i < nv; i++) {
    found[i] = 0;
  }
  found[source] = 1;
  int64_t found_count = 1;

  std::vector<std::set<int64_t> > levels;
  std::set<int64_t> frontier;

  frontier.insert(source);
  LOG_D_A("pushing onto levels, size %ld", levels.size());
  levels.push_back(frontier);
  LOG_D_A("-------      levels, size %ld", levels.size());

  std::set<int64_t>::iterator it;

  while (found_count < limit && !levels.back().empty()) {
    frontier.clear();
    std::set<int64_t>& cur = levels.back();

    for (it = cur.begin(); found_count < limit && it != cur.end(); it++) {
      int64_t v = *it;

      STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, v) {
	if (found_count < limit && !found[STINGER_EDGE_DEST] && labels.equal(source, STINGER_EDGE_DEST)) {
	  frontier.insert(STINGER_EDGE_DEST);
	  found[STINGER_EDGE_DEST] = 1;
	  found_count++;
	}
      } STINGER_FORALL_EDGES_OF_VTX_END();

    }

    LOG_D_A("pushing onto levels, size %ld", levels.size());
    levels.push_back(frontier);
    LOG_D_A("-------      levels, size %ld", levels.size());
  }

  while(!levels.empty()) {
    std::set<int64_t>::iterator cur = levels.back().begin();
    std::set<int64_t>::iterator end = levels.back().end();

    for(; cur != end; cur++) {
      if(get_vtypes) {
	char intstr[21];
	sprintf(intstr, "%ld", (long)*cur);
	vtype.SetInt64(stinger_vtype_get(S, *cur));
	vtypes.AddMember(intstr, vtype, allocator);
	if(strings) {
	  char * name = NULL;
	  uint64_t len = 0;
	  stinger_mapping_physid_direct(S, *cur, &name, &len);
	  char * vtype_name = stinger_vtype_names_lookup_name(S,stinger_vtype_get(S, *cur));
	  vtype.SetString(vtype_name, strlen(vtype_name), allocator);
	  vtx_name.SetString(name, len, allocator);
	  vtypes_str.AddMember(vtx_name, vtype, allocator);
	}
      }
      STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, *cur) {
	if (found[STINGER_EDGE_DEST]) {
	  src.SetInt64(STINGER_EDGE_SOURCE);
	  dst.SetInt64(STINGER_EDGE_DEST);
	  val.SetArray();
	  val.PushBack(src, allocator);
	  val.PushBack(dst, allocator);
	  if(get_etypes) {
	    etype.SetInt64(STINGER_EDGE_TYPE);
	    val.PushBack(etype, allocator);
	  }
	  a.PushBack(val, allocator);
	  if (strings) {
	    char * physID;
	    uint64_t len;
	    if(-1 == stinger_mapping_physid_direct(S, STINGER_EDGE_SOURCE, &physID, &len)) {
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

    levels.pop_back();
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

  free (found);

  return 0;
}


int64_t 
JSON_RPC_get_algorithms::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  return algorithms_to_json(server_state, result, allocator);
}

int
algorithms_to_json (JSON_RPCServerState * server_state,
  rapidjson::Value& rtn,
  rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  rapidjson::Value a(rapidjson::kArrayType);
  rapidjson::Value v;
  
  size_t num_algs = server_state->get_num_algs();

  for (size_t i = 0; i < num_algs; i++) {
    StingerAlgState * alg_state = server_state->get_alg(i);
    const char * alg_name = alg_state->name.c_str();

    v.SetString(alg_name, strlen(alg_name), allocator);
    a.PushBack(v, allocator);
  }

  v.SetString("stinger", strlen("stinger"), allocator);
  a.PushBack(v, allocator);

  rtn.AddMember("algorithms", a, allocator);

  return 0;
}


int64_t 
JSON_RPC_get_data_description::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  char * algorithm_name;
  rpc_params_t p[] = {
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (contains_params(p, params)) {
    if(0 == strcmp("stinger", algorithm_name)) {
	rapidjson::Value a(rapidjson::kArrayType);

	rapidjson::Value v;
	v.SetString("vertex_weight", strlen("vertex_weight"), allocator);
	a.PushBack(v, allocator);
	v.SetString("vertex_type_num", strlen("vertex_type_num"), allocator);
	a.PushBack(v, allocator);
	v.SetString("vertex_type_name", strlen("vertex_type_name"), allocator);
	a.PushBack(v, allocator);
	v.SetString("vertex_indegree", strlen("vertex_indegree"), allocator);
	a.PushBack(v, allocator);
	v.SetString("vertex_outdegree", strlen("vertex_outdegree"), allocator);
	a.PushBack(v, allocator);

	result.AddMember("alg_data", a, allocator);
	return 0;
    } else {
      StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
      if (!alg_state) {
	LOG_E ("Algorithm is not running");
	return json_rpc_error(-32003, result, allocator);
      }
      return description_string_to_json(alg_state->data_description.c_str(), result, allocator);
    }
  } else {
    return json_rpc_error(-32602, result, allocator);
  }

}

int
description_string_to_json (const char * description_string,
  rapidjson::Value& rtn,
  rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Value a(rapidjson::kArrayType);

  /* the description string is space-delimited */
  char * placeholder;
  char * ptr = strtok_r (tmp, " ", &placeholder);

  /* skip the formatting */
  char * pch = strtok_r (NULL, " ", &placeholder);

  while (pch != NULL)
  {
    rapidjson::Value v;
    v.SetString(pch, strlen(pch), allocator);
    a.PushBack(v, allocator);

    pch = strtok_r (NULL, " ", &placeholder);
  }

  rtn.AddMember("alg_data", a, allocator);

  free(tmp);
  return 0;
}


int64_t 
JSON_RPC_get_data_array_range::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  char * algorithm_name;
  char * data_array_name;
  int64_t stride, nsamples;
  int64_t count, offset;
  bool strings, logscale;
  rpc_params_t p[] = {
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {"data", TYPE_STRING, &data_array_name, false, 0},
    {"offset", TYPE_INT64, &offset, false, 0},
    {"count", TYPE_INT64, &count, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {"stride", TYPE_INT64, &stride, true, 1},
    {"samples", TYPE_INT64, &nsamples, true, 0},
    {"log", TYPE_BOOL, &logscale, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  rapidjson::Value max_time_seen;
  max_time_seen.SetInt64(server_state->get_max_time());

  if (contains_params(p, params)) {
    StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
    if (nsamples) {
      stride = (count + nsamples - 1) / nsamples;
    }
    if (!alg_state) {
      if(0 != strcmp("stinger", algorithm_name)) {
	LOG_E ("Algorithm is not running");
	return json_rpc_error(-32003, result, allocator);
      } else {
	result.AddMember("time", max_time_seen, allocator);
	return array_to_json_monolithic_stinger (
	    RANGE,
	    server_state->get_stinger(),
	    result,
	    allocator,
	    NULL, //alg_state->data_description.c_str(),
	    stinger_mapping_nv(server_state->get_stinger()),
	    NULL, //(uint8_t *) alg_state->data,
	    strings,
	    data_array_name,
	    stride,
	    logscale,
	    offset,
	    offset+count
	);
      }
    }
    result.AddMember("time", max_time_seen, allocator);
    return array_to_json_monolithic (
	RANGE,
	server_state->get_stinger(),
	result,
	allocator,
	alg_state->data_description.c_str(),
	stinger_mapping_nv(server_state->get_stinger()),
	(uint8_t *) alg_state->data,
	strings,
	data_array_name,
	stride,
	logscale,
	offset,
	offset+count
    );
  } else {
    return json_rpc_error(-32602, result, allocator);
  }
}


int64_t 
JSON_RPC_get_data_array_sorted_range::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  char * algorithm_name;
  char * data_array_name;
  int64_t stride, nsamples;
  int64_t count, offset;
  char * order;
  bool strings, logscale;
  rpc_params_t p[] = {
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {"data", TYPE_STRING, &data_array_name, false, 0},
    {"offset", TYPE_INT64, &offset, false, 0},
    {"count", TYPE_INT64, &count, false, 0},
    {"order", TYPE_STRING, &order, true, (int64_t)"DESC"},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {"stride", TYPE_INT64, &stride, true, 1},
    {"samples", TYPE_INT64, &nsamples, true, 0},
    {"log", TYPE_BOOL, &logscale, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  rapidjson::Value max_time_seen;
  max_time_seen.SetInt64(server_state->get_max_time());

  if (contains_params(p, params)) {
    StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
    if (nsamples) {
      stride = (count + nsamples - 1) / nsamples;
    }
    if (!alg_state) {
      if(0 != strcmp("stinger", algorithm_name)) {
	LOG_E ("Algorithm is not running");
	return json_rpc_error(-32003, result, allocator);
      } else {
	result.AddMember("time", max_time_seen, allocator);
	return array_to_json_monolithic_stinger (
	  SORTED,
	  server_state->get_stinger(),
	  result,
	  allocator,
	  NULL, // alg_state->data_description.c_str(),
	  stinger_mapping_nv(server_state->get_stinger()),
	  NULL, // (uint8_t *) alg_state->data,
	  strings,
	  data_array_name,
	  stride,
	  logscale,
	  offset,
	  offset+count,
	  order
	);
      }
    }
    result.AddMember("time", max_time_seen, allocator);
    return array_to_json_monolithic (
	SORTED,
	server_state->get_stinger(),
	result,
	allocator,
	alg_state->data_description.c_str(),
	stinger_mapping_nv(server_state->get_stinger()),
	(uint8_t *) alg_state->data,
	strings,
	data_array_name,
	stride,
	logscale,
	offset,
	offset+count,
	order
    );
  } else {
    return json_rpc_error(-32602, result, allocator);
  }
}


int64_t 
JSON_RPC_get_data_array_set::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  char * algorithm_name;
  char * data_array_name;
  params_array_t set_array;
  bool strings;
  rpc_params_t p[] = {
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {"data", TYPE_STRING, &data_array_name, false, 0},
    {"set", TYPE_ARRAY, &set_array, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  rapidjson::Value max_time_seen;
  max_time_seen.SetInt64(server_state->get_max_time());

  if (contains_params(p, params)) {
    StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
    if (!alg_state) {
      if(0 != strcmp("stinger", algorithm_name)) {
	LOG_E ("Algorithm is not running");
	return json_rpc_error(-32003, result, allocator);
      } else {
	result.AddMember("time", max_time_seen, allocator);
	return array_to_json_monolithic_stinger (
	  SET,
	  server_state->get_stinger(),
	  result,
	  allocator,
	  NULL, //alg_state->data_description.c_str(),
	  stinger_mapping_nv(server_state->get_stinger()),
	  NULL, //(uint8_t *) alg_state->data,
	  strings,
	  data_array_name,
	  1,
	  false,
	  0,
	  0,
	  NULL,
	  set_array.arr,
	  set_array.len
	);
      }
    }
    result.AddMember("time", max_time_seen, allocator);
    return array_to_json_monolithic (
	SET,
	server_state->get_stinger(),
	result,
	allocator,
	alg_state->data_description.c_str(),
	stinger_mapping_nv(server_state->get_stinger()),
	(uint8_t *) alg_state->data,
	strings,
	data_array_name,
	1,
	false,
	0,
	0,
	NULL,
	set_array.arr,
	set_array.len
    );
  } else {
    LOG_W ("didn't have the right params");
    return json_rpc_error(-32602, result, allocator);
  }
}


int64_t 
JSON_RPC_get_data_array::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  char * algorithm_name;
  char * data_array_name;
  int64_t stride, nsamples;
  bool strings, logscale;
  rpc_params_t p[] = {
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {"data", TYPE_STRING, &data_array_name, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {"stride", TYPE_INT64, &stride, true, 1},
    {"samples", TYPE_INT64, &nsamples, true, 0},
    {"log", TYPE_BOOL, &logscale, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  rapidjson::Value max_time_seen;
  max_time_seen.SetInt64(server_state->get_max_time());

  if (contains_params(p, params)) {
    StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
    if (nsamples) {
      stride = (stinger_mapping_nv(server_state->get_stinger()) + nsamples - 1) / nsamples;
    }
    if (!alg_state) {
      if(0 != strcmp("stinger", algorithm_name)) {
	LOG_E ("Algorithm is not running");
	return json_rpc_error(-32003, result, allocator);
      } else {
	result.AddMember("time", max_time_seen, allocator);
	return array_to_json_monolithic_stinger (
	  RANGE,
	  server_state->get_stinger(),
	  result,
	  allocator,
	  NULL, //alg_state->data_description.c_str(),
	  stinger_mapping_nv(server_state->get_stinger()),
	  NULL, //(uint8_t *) alg_state->data,
	  strings,
	  data_array_name,
	  stride,
	  logscale,
	  0,
	  stinger_mapping_nv(server_state->get_stinger())
	);
      }
    }
    result.AddMember("time", max_time_seen, allocator);
    return array_to_json_monolithic (
	RANGE,
	server_state->get_stinger(),
	result,
	allocator,
	alg_state->data_description.c_str(),
	stinger_mapping_nv(server_state->get_stinger()),
	(uint8_t *) alg_state->data,
	strings,
	data_array_name,
	stride,
	logscale,
	0,
	stinger_mapping_nv(server_state->get_stinger())
    );
  } else {
    return json_rpc_error(-32602, result, allocator);
  }
}


/* Big monolithic function that does everything related to the data array */
int
array_to_json_monolithic   (json_rpc_array_meth_t method, stinger_t * S,
			    rapidjson::Value& rtn,
			    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator,
			    const char * description_string, int64_t nv, uint8_t * data,
			    bool strings,
			    const char * search_string,
			    int64_t stride,
			    bool logscale,
			    int64_t start, int64_t end,
			    const char * order_str,
			    int64_t * set, int64_t set_len
			    )
{
  if (method == SET) {
    if (set_len < 1) {
      LOG_E_A("Invalid set length: %ld.", set_len);
    }
    if (!set) {
      LOG_E("Vertex set is null.");
    }
  }
  if (method == SORTED || method == RANGE) {
    if (start >= nv) {
      LOG_E_A("Invalid range: %ld to %ld. Expecting [0, %ld).", start, end, nv);
      return json_rpc_error(-32602, rtn, allocator);
    }
    if (end > nv) {
      end = nv;
      LOG_W_A("Invalid end of range: %ld. Expecting less than %ld.", end, nv);
    }
  }
  if (!S & strings) {
    LOG_E("STINGER pointer must be valid in order to process strings");
    return json_rpc_error(-32603, rtn, allocator);
  }
  if (stride <= 0) {
    LOG_W_A("Stride of %ld is not allowed. Fixing.", stride);
    stride = 1;
  }
  if (stride >= nv) {
    LOG_W_A("Stride of %ld only returns one value. This probably isn't what you want.", stride);
  }
  
  bool asc;
  if (method == SORTED) {
    if (strncmp(order_str, "ASC", 3)==0) {
      asc = true;
    }
    else if (strncmp(order_str, "DESC", 4)==0) {
      asc = false;
    }
    else {
      return json_rpc_error(-32603, rtn, allocator);
    }
  }

  if (method == SET) {
    start = 0;
    end = set_len;
  }

  int64_t nsamples = (end - start + 1)/(stride);

  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Value result (rapidjson::kObjectType);
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_val (rapidjson::kArrayType);
  rapidjson::Value vtx_str (rapidjson::kArrayType);

  /* the description string is space-delimited */
  char * placeholder;
  char * ptr = strtok_r (tmp, " ", &placeholder);

  /* skip the formatting */
  char * pch = strtok_r (NULL, " ", &placeholder);

  int64_t done = 0;

  LOG_D_A ("%s :: %s", description_string, search_string);
  if (pch == NULL) {
    LOG_W_A ("pch is null :: %s :: %s", description_string, search_string);
  }

  while (pch != NULL)
  {
    LOG_D_A ("%s: begin while :: %s", search_string, pch);
    if (strcmp(pch, search_string) == 0) {
      LOG_D_A ("%s: matches", search_string);
      switch (description_string[off]) {
	case 'f':
	  {
	    LOG_D_A ("%s: case f", search_string);
	    int64_t * idx;
	    if (method == SORTED) {
	      idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	      for (int64_t i = 0; i < nv; i++) {
		idx[i] = i;
	      }

	      if (asc) {
		compare_pair_asc<float> comparator((float *) data);
		std::sort(idx, idx + nv, comparator);
	      } else {
		compare_pair_desc<float> comparator((float *) data);
		std::sort(idx, idx + nv, comparator);
	      }
	    }

	    rapidjson::Value value, name, vtx_phys;
	    double factor = pow((double)(end - start), 1.0 /(double)nsamples);
	    for (double i = start; i < end; i += stride) {
	      if (logscale) {
		if (i != start) {
		  i -= stride;
		  int64_t prev = i;
		  if (i != start) {
		    i = pow (factor, log((double) (i - start)) / log (factor) + 1);
		  } else {
		    i = pow (factor, 1);
		  }
		  if (prev == ((int64_t) i))
		    continue;
		} 
	      }

	      int64_t vtx;
	      if (method == SORTED)
		vtx = idx[(int64_t)i];
	      if (method == RANGE)
		vtx = i;
	      if (method == SET)
		vtx = set[(int64_t)i];
	      value.SetDouble((double)((float *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);
	      
	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);

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

	    if (method == SORTED)
	      free(idx);
	    done = 1;
	    break;
	  }

	case 'd':
	  {
	    LOG_D_A ("%s: case d", search_string);
	    int64_t * idx;
	    if (method == SORTED) {
	      idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	      for (int64_t i = 0; i < nv; i++) {
		idx[i] = i;
	      }

	      if (asc) {
		compare_pair_asc<double> comparator((double *) data);
		std::sort(idx, idx + nv, comparator);
	      } else {
		compare_pair_desc<double> comparator((double *) data);
		std::sort(idx, idx + nv, comparator);
	      }
	    }

	    rapidjson::Value value, name, vtx_phys;
	    double factor = pow((double)(end - start), 1.0 /(double)nsamples);
	    for (double i = start; i < end; i += stride) {
	      if (logscale) {
		if (i != start) {
		  i -= stride;
		  int64_t prev = i;
		  if (i != start) {
		    i = pow (factor, log ((double) (i - start)) / log (factor) + 1);
		  } else {
		    i = pow (factor, 1);
		  }
		  if (prev == ((int64_t) i))
		    continue;
		} 
	      }

	      int64_t vtx;
	      if (method == SORTED)
		vtx = idx[(int64_t)i];
	      if (method == RANGE)
		vtx = i;
	      if (method == SET)
		vtx = set[(int64_t)i];
	      value.SetDouble((double)((double *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);

	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);

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

	    if (method == SORTED)
	      free(idx);
	    done = 1;
	    break;
	  }

	case 'i':
	  {
	    LOG_D_A ("%s: case i", search_string);
	    int64_t * idx;
	    if (method == SORTED) {
	      idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	      for (int64_t i = 0; i < nv; i++) {
		idx[i] = i;
	      }

	      if (asc) {
		compare_pair_asc<int32_t> comparator((int32_t *) data);
		std::sort(idx, idx + nv, comparator);
	      } else {
		compare_pair_desc<int32_t> comparator((int32_t *) data);
		std::sort(idx, idx + nv, comparator);
	      }
	    }

	    rapidjson::Value value, name, vtx_phys;
	    double factor = pow((double)(end - start), 1.0 /(double)nsamples);
	    for (double i = start; i < end; i += stride) {
	      if (logscale) {
		if (i != start) {
		  i -= stride;
		  int64_t prev = i;
		  if (i != start) {
		    i = pow (factor, log ((double) (i - start)) / log (factor) + 1);
		  } else {
		    i = pow (factor, 1);
		  }
		  if (prev == ((int64_t) i))
		    continue;
		} 
	      }

	      int64_t vtx;
	      if (method == SORTED)
		vtx = idx[(int64_t)i];
	      if (method == RANGE)
		vtx = i;
	      if (method == SET)
		vtx = set[(int64_t)i];
	      value.SetInt((int)((int32_t *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);

	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);

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

	    if (method == SORTED)
	      free(idx);
	    done = 1;
	    break;
	  }

	case 'l':
	  {
	    LOG_D_A ("%s: case l", search_string);
	    int64_t * idx;
	    if (method == SORTED) {
	      idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	      for (int64_t i = 0; i < nv; i++) {
		idx[i] = i;
	      }

	      if (asc) {
		compare_pair_asc<int64_t> comparator((int64_t *) data);
		std::sort(idx, idx + nv, comparator);
	      } else {
		compare_pair_desc<int64_t> comparator((int64_t *) data);
		std::sort(idx, idx + nv, comparator);
	      }
	    }

	    rapidjson::Value value, name, vtx_phys;
	    double factor = pow((double)(end - start), 1.0 /(double)nsamples);
	    for (double i = start; i < end; i += stride) {
	      if (logscale) {
		if (i != start) {
		  i -= stride;
		  int64_t prev = i;
		  if (i != start) {
		    i = pow (factor, log ((double) (i - start)) / log (factor) + 1);
		  } else {
		    i = pow (factor, 1);
		  }
		  if (prev == ((int64_t) i))
		    continue;
		} 
	      }

	      int64_t vtx;
	      if (method == SORTED)
		vtx = idx[(int64_t)i];
	      if (method == RANGE)
		vtx = i;
	      if (method == SET)
		vtx = set[(int64_t)i];
	      value.SetInt64((int64_t)((int64_t *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);

	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);

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

	    if (method == SORTED)
	      free(idx);
	    done = 1;
	    break;
	  }

	case 'b':
	  {
	    LOG_D_A ("%s: case b", search_string);
	    int64_t * idx;
	    if (method == SORTED) {
	      idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	      for (int64_t i = 0; i < nv; i++) {
		idx[i] = i;
	      }

	      if (asc) {
		compare_pair_asc<uint8_t> comparator((uint8_t *) data);
		std::sort(idx, idx + nv, comparator);
	      } else {
		compare_pair_desc<uint8_t> comparator((uint8_t *) data);
		std::sort(idx, idx + nv, comparator);
	      }
	    }

	    rapidjson::Value value, name, vtx_phys;
	    double factor = pow((double)(end - start), 1.0 /(double)nsamples);
	    for (double i = start; i < end; i += stride) {
	      if (logscale) {
		if (i != start) {
		  i -= stride;
		  int64_t prev = i;
		  if(i != start) {
		    i = pow (factor, log ((double) (i - start)) / log (factor) + 1);
		  } else {
		    i = pow (factor, 1);
		  }
		  if (prev == ((int64_t) i))
		    continue;
		} 
	      }

	      int64_t vtx;
	      if (method == SORTED)
		vtx = idx[(int64_t)i];
	      if (method == RANGE)
		vtx = i;
	      if (method == SET)
		vtx = set[(int64_t)i];
	      value.SetInt((int)((uint8_t *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);

	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);

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

	    if (method == SORTED)
	      free(idx);
	    done = 1;
	    break;
	  }

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }
      


    } else {
      LOG_D_A ("%s: does not match %d", search_string, nv);
      switch (description_string[off]) {
	case 'f':
	  data += (S->max_nv * sizeof(float));
	  break;

	case 'd':
	  data += (S->max_nv * sizeof(double));
	  break;

	case 'i':
	  data += (S->max_nv * sizeof(int32_t));
	  break;

	case 'l':
	  data += (S->max_nv * sizeof(int64_t));
	  break;

	case 'b':
	  data += (S->max_nv * sizeof(uint8_t));
	  break;

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }
      off++;
    }

    LOG_D_A ("%s: done: %d", search_string, done);
    
    if (done)
      break;

    pch = strtok_r (NULL, " ", &placeholder);
  }

  free(tmp);
  if (done) {
    rapidjson::Value offset, count, order;
    if (method == SORTED || method == RANGE) {
      offset.SetInt64(start);
      result.AddMember("offset", offset, allocator);
      count.SetInt64(end-start);
      result.AddMember("count", count, allocator);
    }
    if (method == SORTED) {
      order.SetString(order_str, strlen(order_str), allocator);
      result.AddMember("order", order, allocator);
    }
    result.AddMember("vertex_id", vtx_id, allocator);
    if (strings)
      result.AddMember("vertex_str", vtx_str, allocator);
    result.AddMember("value", vtx_val, allocator);

    rtn.AddMember(search_string, result, allocator);
  }
  else {
    LOG_W_A ("%s: shouldn't get here", search_string);
    return json_rpc_error(-32602, rtn, allocator);
  }

  return 0;
}

/* Function with the same interface as the other monotlithic, but created to supply 
 * data from stinger */
int
array_to_json_monolithic_stinger   (json_rpc_array_meth_t method, stinger_t * S,
			    rapidjson::Value& rtn,
			    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator,
			    const char * description_string, int64_t nv, uint8_t * data,
			    bool strings,
			    const char * search_string,
			    int64_t stride,
			    bool logscale,
			    int64_t start, int64_t end,
			    const char * order_str,
			    int64_t * set, int64_t set_len
			    )
{
  if (method == SET) {
    if (set_len < 1) {
      LOG_E_A("Invalid set length: %ld.", set_len);
    }
    if (!set) {
      LOG_E("Vertex set is null.");
    }
  }
  if (method == SORTED || method == RANGE) {
    if (start >= nv) {
      LOG_E_A("Invalid range: %ld to %ld. Expecting [0, %ld).", start, end, nv);
      return json_rpc_error(-32602, rtn, allocator);
    }
    if (end > nv) {
      end = nv;
      LOG_W_A("Invalid end of range: %ld. Expecting less than %ld.", end, nv);
    }
  }
  if (!S & strings) {
    LOG_E("STINGER pointer must be valid in order to process strings");
    return json_rpc_error(-32603, rtn, allocator);
  }
  if (stride <= 0) {
    LOG_W_A("Stride of %ld is not allowed. Fixing.", stride);
    stride = 1;
  }
  if (stride >= nv) {
    LOG_W_A("Stride of %ld only returns one value. This probably isn't what you want.", stride);
  }
  
  bool asc;
  if (method == SORTED) {
    if (strncmp(order_str, "ASC", 3)==0) {
      asc = true;
    }
    else if (strncmp(order_str, "DESC", 4)==0) {
      asc = false;
    }
    else {
      return json_rpc_error(-32603, rtn, allocator);
    }
  }

  if (method == SET) {
    start = 0;
    end = set_len;
  }

  int64_t nsamples = (end - start + 1)/(stride);

  size_t off = 0;

  rapidjson::Value result (rapidjson::kObjectType);
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_val (rapidjson::kArrayType);
  rapidjson::Value vtx_str (rapidjson::kArrayType);

  int64_t done = 0;

  LOG_D_A ("%s", search_string);

  MAP_STING(S);

  if(0 == strcmp(search_string, "vertex_outdegree")) {
      int64_t * idx;
      if (method == SORTED) {
	idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	for (int64_t i = 0; i < nv; i++) {
	  idx[i] = i;
	}

	if (asc) {
	  compare_pair_off_asc<int64_t> comparator(&(vertices->vertices[0].outDegree), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	} else {
	  compare_pair_off_desc<int64_t> comparator(&(vertices->vertices[0].outDegree), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	}
      }

      rapidjson::Value value, name, vtx_phys;
      double factor = pow((double)(end - start), 1.0 /(double)nsamples);
      for (double i = start; i < end; i += stride) {
	if (logscale) {
	  if (i != start) {
	    i -= stride;
	    int64_t prev = i;
	    if (i != start) {
	      i = pow (factor, log((double) (i - start)) / log (factor) + 1);
	    } else {
	      i = pow (factor, 1);
	    }
	    if (prev == ((int64_t) i))
	      continue;
	  } 
	}

	int64_t vtx;
	if (method == SORTED)
	  vtx = idx[(int64_t)i];
	if (method == RANGE)
	  vtx = i;
	if (method == SET)
	  vtx = set[(int64_t)i];
	value.SetInt64(stinger_outdegree(S, vtx));
	vtx_val.PushBack(value, allocator);
	
	name.SetInt64(vtx);
	vtx_id.PushBack(name, allocator);

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

      if (method == SORTED)
	free(idx);
      done = 1;
  } else if(0 == strcmp(search_string, "vertex_indegree")) {
      int64_t * idx;
      if (method == SORTED) {
	idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	for (int64_t i = 0; i < nv; i++) {
	  idx[i] = i;
	}

	if (asc) {
	  compare_pair_off_asc<int64_t> comparator(&(vertices->vertices[0].inDegree), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	} else {
	  compare_pair_off_desc<int64_t> comparator(&(vertices->vertices[0].inDegree), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	}
      }

      rapidjson::Value value, name, vtx_phys;
      double factor = pow((double)(end - start), 1.0 /(double)nsamples);
      for (double i = start; i < end; i += stride) {
	if (logscale) {
	  if (i != start) {
	    i -= stride;
	    int64_t prev = i;
	    if (i != start) {
	      i = pow (factor, log((double) (i - start)) / log (factor) + 1);
	    } else {
	      i = pow (factor, 1);
	    }
	    if (prev == ((int64_t) i))
	      continue;
	  } 
	}

	int64_t vtx;
	if (method == SORTED)
	  vtx = idx[(int64_t)i];
	if (method == RANGE)
	  vtx = i;
	if (method == SET)
	  vtx = set[(int64_t)i];
	value.SetInt64(stinger_indegree(S, vtx));
	vtx_val.PushBack(value, allocator);
	
	name.SetInt64(vtx);
	vtx_id.PushBack(name, allocator);

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

      if (method == SORTED)
	free(idx);
      done = 1;
  } else if(0 == strcmp(search_string, "vertex_weight")) {
      int64_t * idx;
      if (method == SORTED) {
	idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	for (int64_t i = 0; i < nv; i++) {
	  idx[i] = i;
	}

	if (asc) {
	  compare_pair_off_asc<int64_t> comparator(&(vertices->vertices[0].weight), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	} else {
	  compare_pair_off_desc<int64_t> comparator(&(vertices->vertices[0].weight), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	}
      }

      rapidjson::Value value, name, vtx_phys;
      double factor = pow((double)(end - start), 1.0 /(double)nsamples);
      for (double i = start; i < end; i += stride) {
	if (logscale) {
	  if (i != start) {
	    i -= stride;
	    int64_t prev = i;
	    if (i != start) {
	      i = pow (factor, log((double) (i - start)) / log (factor) + 1);
	    } else {
	      i = pow (factor, 1);
	    }
	    if (prev == ((int64_t) i))
	      continue;
	  } 
	}

	int64_t vtx;
	if (method == SORTED)
	  vtx = idx[(int64_t)i];
	if (method == RANGE)
	  vtx = i;
	if (method == SET)
	  vtx = set[(int64_t)i];
	value.SetInt64(stinger_vweight_get(S, vtx));
	vtx_val.PushBack(value, allocator);
	
	name.SetInt64(vtx);
	vtx_id.PushBack(name, allocator);

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

      if (method == SORTED)
	free(idx);
      done = 1;
  } else if(0 == strcmp(search_string, "vertex_type_num")) {
      int64_t * idx;
      if (method == SORTED) {
	idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	for (int64_t i = 0; i < nv; i++) {
	  idx[i] = i;
	}

	if (asc) {
	  compare_pair_off_asc<int64_t> comparator(&(vertices->vertices[0].type), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	} else {
	  compare_pair_off_desc<int64_t> comparator(&(vertices->vertices[0].type), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	}
      }

      rapidjson::Value value, name, vtx_phys;
      double factor = pow((double)(end - start), 1.0 /(double)nsamples);
      for (double i = start; i < end; i += stride) {
	if (logscale) {
	  if (i != start) {
	    i -= stride;
	    int64_t prev = i;
	    if (i != start) {
	      i = pow (factor, log((double) (i - start)) / log (factor) + 1);
	    } else {
	      i = pow (factor, 1);
	    }
	    if (prev == ((int64_t) i))
	      continue;
	  } 
	}

	int64_t vtx;
	if (method == SORTED)
	  vtx = idx[(int64_t)i];
	if (method == RANGE)
	  vtx = i;
	if (method == SET)
	  vtx = set[(int64_t)i];
	value.SetInt64(stinger_vtype_get(S, vtx));
	vtx_val.PushBack(value, allocator);
	
	name.SetInt64(vtx);
	vtx_id.PushBack(name, allocator);

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

      if (method == SORTED)
	free(idx);
      done = 1;
  } else if(0 == strcmp(search_string, "vertex_type_name")) {
      int64_t * idx;
      if (method == SORTED) {
	idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	for (int64_t i = 0; i < nv; i++) {
	  idx[i] = i;
	}

	if (asc) {
	  compare_pair_off_asc<int64_t> comparator(&(vertices->vertices[0].type), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	} else {
	  compare_pair_off_desc<int64_t> comparator(&(vertices->vertices[0].type), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	}
      }

      rapidjson::Value value, name, vtx_phys;
      double factor = pow((double)(end - start), 1.0 /(double)nsamples);
      for (double i = start; i < end; i += stride) {
	if (logscale) {
	  if (i != start) {
	    i -= stride;
	    int64_t prev = i;
	    if (i != start) {
	      i = pow (factor, log((double) (i - start)) / log (factor) + 1);
	    } else {
	      i = pow (factor, 1);
	    }
	    if (prev == ((int64_t) i))
	      continue;
	  } 
	}

	int64_t vtx;
	if (method == SORTED)
	  vtx = idx[(int64_t)i];
	if (method == RANGE)
	  vtx = i;
	if (method == SET)
	  vtx = set[(int64_t)i];
	value.SetString(stinger_vtype_names_lookup_name(S,stinger_vtype_get(S, vtx)));
	vtx_val.PushBack(value, allocator);
	
	name.SetInt64(vtx);
	vtx_id.PushBack(name, allocator);

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

      if (method == SORTED)
	free(idx);
      done = 1;
  } else { 
  }


  if (done) {
    rapidjson::Value offset, count, order;
    if (method == SORTED || method == RANGE) {
      offset.SetInt64(start);
      result.AddMember("offset", offset, allocator);
      count.SetInt64(end-start);
      result.AddMember("count", count, allocator);
    }
    if (method == SORTED) {
      order.SetString(order_str, strlen(order_str), allocator);
      result.AddMember("order", order, allocator);
    }
    result.AddMember("vertex_id", vtx_id, allocator);
    if (strings)
      result.AddMember("vertex_str", vtx_str, allocator);
    result.AddMember("value", vtx_val, allocator);

    rtn.AddMember(search_string, result, allocator);
  }
  else {
    LOG_W_A ("%s: shouldn't get here", search_string);
    return json_rpc_error(-32602, rtn, allocator);
  }

  return 0;
}


int64_t 
JSON_RPC_get_data_array_reduction::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  char * algorithm_name;
  char * reduce_op;
  rpc_params_t p[] = {
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {"op", TYPE_STRING, &reduce_op, false, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  rapidjson::Value max_time_seen;
  max_time_seen.SetInt64(server_state->get_max_time());

  if (contains_params(p, params)) {
    StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
    if (!alg_state) {
      LOG_E ("Algorithm is not running");
      return json_rpc_error(-32003, result, allocator);
    }
    result.AddMember("time", max_time_seen, allocator);
    return array_to_json_reduction (
	server_state->get_stinger(),
	result,
	allocator,
	alg_state->data_description.c_str(),
	(uint8_t *) alg_state->data,
	algorithm_name
    );
  } else {
    return json_rpc_error(-32602, result, allocator);
  }
}


int
array_to_json_reduction    (stinger_t * S,
			    rapidjson::Value& rtn,
			    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator,
			    const char * description_string, uint8_t * data,
			    const char * algorithm_name
			    )
{
  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Value fields (rapidjson::kArrayType);

  /* the description string is space-delimited */
  char * placeholder;
  char * ptr = strtok_r (tmp, " ", &placeholder);

  /* skip the formatting */
  char * pch = strtok_r (NULL, " ", &placeholder);

  if (pch == NULL) {
    LOG_W_A ("pch is null :: %s :: %s", description_string, algorithm_name);
  }

  int64_t start = 0;
  int64_t end = S->max_nv;

  while (pch != NULL)
  {
      size_t pch_len = strlen(pch);
      switch (description_string[off]) {
	case 'f':
	  {
	    rapidjson::Value reduction (rapidjson::kObjectType);
	    rapidjson::Value reduction_name, reduction_value;
	    double sum = 0.0;
	    for (int64_t i = start; i < end; i++) {
	      double val = (double)((float *) data)[i];
	      if (val > 0.0)
		sum += val;
	    }
	    reduction_value.SetDouble(sum);
	    reduction_name.SetString(pch, pch_len, allocator);
	    
	    reduction.AddMember("field", reduction_name, allocator);
	    reduction.AddMember("value", reduction_value, allocator);

	    fields.PushBack(reduction, allocator);
	    break;
	  }

	case 'd':
	  {
	    rapidjson::Value reduction (rapidjson::kObjectType);
	    rapidjson::Value reduction_name, reduction_value;
	    double sum = 0.0;
	    for (int64_t i = start; i < end; i++) {
	      double val = (double)((double *) data)[i];
	      if (val > 0.0)
		sum += val;
	    }
	    reduction_value.SetDouble(sum);
	    reduction_name.SetString(pch, pch_len, allocator);
	    
	    reduction.AddMember("field", reduction_name, allocator);
	    reduction.AddMember("value", reduction_value, allocator);

	    fields.PushBack(reduction, allocator);
	    break;
	  }

	case 'i':
	  {
	    rapidjson::Value reduction (rapidjson::kObjectType);
	    rapidjson::Value reduction_name, reduction_value;
	    int64_t sum = 0;
	    for (int64_t i = start; i < end; i++) {
	      int64_t val = (int64_t)((int32_t *) data)[i];
	      if (val > 0)
		sum += val;
	    }
	    reduction_value.SetInt64(sum);
	    reduction_name.SetString(pch, pch_len, allocator);
	    
	    reduction.AddMember("field", reduction_name, allocator);
	    reduction.AddMember("value", reduction_value, allocator);

	    fields.PushBack(reduction, allocator);
	    break;
	  }

	case 'l':
	  {
	    rapidjson::Value reduction (rapidjson::kObjectType);
	    rapidjson::Value reduction_name, reduction_value;
	    int64_t sum = 0;
	    for (int64_t i = start; i < end; i++) {
	      int64_t val = (int64_t)((int64_t *) data)[i];
	      if (val > 0)
		sum += val;
	    }
	    
	    reduction_value.SetInt64(sum);
	    reduction_name.SetString(pch, pch_len, allocator);
	    
	    reduction.AddMember("field", reduction_name, allocator);
	    reduction.AddMember("value", reduction_value, allocator);

	    fields.PushBack(reduction, allocator);
	    break;
	  }

	case 'b':
	  {
	    rapidjson::Value reduction (rapidjson::kObjectType);
	    rapidjson::Value reduction_name, reduction_value;
	    int64_t sum = 0;
	    for (int64_t i = start; i < end; i++) {
	      int64_t val = (int64_t)((uint8_t *) data)[i];
	      if (val > 0)
		sum += val;
	    }

	    reduction_value.SetInt64(sum);
	    reduction_name.SetString(pch, pch_len, allocator);
	    
	    reduction.AddMember("field", reduction_name, allocator);
	    reduction.AddMember("value", reduction_value, allocator);
	    
	    fields.PushBack(reduction, allocator);
	    break;
	  }

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }

      switch (description_string[off]) {
	case 'f':
	  data += (S->max_nv * sizeof(float));
	  break;

	case 'd':
	  data += (S->max_nv * sizeof(double));
	  break;

	case 'i':
	  data += (S->max_nv * sizeof(int32_t));
	  break;

	case 'l':
	  data += (S->max_nv * sizeof(int64_t));
	  break;

	case 'b':
	  data += (S->max_nv * sizeof(uint8_t));
	  break;

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }
      off++;
    
    pch = strtok_r (NULL, " ", &placeholder);
  }

  free(tmp);
  rtn.AddMember(algorithm_name, fields, allocator);

  return 0;
}

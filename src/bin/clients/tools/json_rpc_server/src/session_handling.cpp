#include <cstdlib>
#include <cstdio>
#include <set>

//#define LOG_AT_W  /* warning only */

extern "C" {
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
}

#include "rapidjson/document.h"

#include "session_handling.h"
#include "json_rpc.h"
#include "alg_data_array.h"

using namespace gt::stinger;


rpc_params_t *
JSON_RPC_community_subgraph::get_params()
{
  return p;
}


int64_t
JSON_RPC_community_subgraph::update(StingerBatch & batch)
{
}


int64_t
JSON_RPC_community_subgraph::onRequest(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  rapidjson::Value insertions, deletions;
  rapidjson::Value src, dst, edge;
  std::set<std::pair<int64_t, int64_t> >::iterator it;

  /* send insertions back */
  insertions.SetArray();

  for (it = _insertions.begin(); it != _insertions.end(); ++it) {
    src.SetInt64((*it).first);
    dst.SetInt64((*it).second);
    edge.SetArray();
    edge.PushBack(src, allocator);
    edge.PushBack(dst, allocator);
    insertions.PushBack(edge, allocator);
  }

  result.AddMember("insertions", insertions, allocator);

  /* send deletions back */

  deletions.SetArray();

  for (it = _deletions.begin(); it != _deletions.end(); ++it) {
    src.SetInt64((*it).first);
    dst.SetInt64((*it).second);
    edge.SetArray();
    edge.PushBack(src, allocator);
    edge.PushBack(dst, allocator);
    deletions.PushBack(edge, allocator);
  }

  result.AddMember("deletions", deletions, allocator);

  /* clear both and reset the clock */
  _insertions.clear();
  _deletions.clear();
  reset_timeout();

  return 0;
}


int64_t
JSON_RPC_community_subgraph::onRegister(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return json_rpc_error(-32603, result, allocator);
  }

  StingerAlgState * alg_state = server_state->get_alg(_algorithm_name);
  if (!alg_state) {
    LOG_E ("Algorithm is not running");
    return json_rpc_error(-32603, result, allocator);
  }

  const char * description_string = alg_state -> data_description.c_str();
  int64_t nv = STINGER_MAX_LVERTICES;
  uint8_t * data = (uint8_t *) alg_state->data;
  const char * search_string = _data_array_name;

  AlgDataArray * df = description_string_to_pointer (description_string, data, nv, search_string);

  if (df->type() != 'l') {
    return json_rpc_error(-32602, result, allocator);
  }

  /* Get the community label of the source vertex */
  int64_t community_id = df->get_int64(_source);

  /* Add all vertices with the same label to the vertices[] set */
  for (int64_t i = 0; i < df->length(); i++) {
    if (df->equal(_source, i)) {
      _vertices.insert(i);
    }
  }

  rapidjson::Value a (rapidjson::kArrayType);
  rapidjson::Value src, dst, edge;

  /* Get all edges within the community */
  std::set<int64_t>::iterator it;
  for (it = _vertices.begin(); it != _vertices.end(); ++it) {
    STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, *it) {
      if (_vertices.find(STINGER_EDGE_DEST) != _vertices.end()) {
	// edge is in the community
	src.SetInt64(*it);
	dst.SetInt64(STINGER_EDGE_DEST);
	edge.SetArray();
	edge.PushBack(src, allocator);
	edge.PushBack(dst, allocator);
	a.PushBack(edge, allocator);
      }
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  result.AddMember("subgraph", a, allocator);

  return 0;
}


AlgDataArray *
description_string_to_pointer (const char * description_string,
				uint8_t * data,
				int64_t nv,
				const char * search_string)
{
  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  /* the description string is space-delimited */
  char * ptr = strtok (tmp, " ");

  /* skip the formatting */
  char * pch = strtok (NULL, " ");

  while (pch != NULL)
  {
    if (strcmp(pch, search_string) == 0) {
      AlgDataArray * rtn = new AlgDataArray(data, description_string[off], nv);
      free (tmp);
      return rtn;

    } else {
      switch (description_string[off]) {
	case 'f':
	  data += (nv * sizeof(float));
	  break;

	case 'd':
	  data += (nv * sizeof(double));
	  break;

	case 'i':
	  data += (nv * sizeof(int32_t));
	  break;

	case 'l':
	  data += (nv * sizeof(int64_t));
	  break;

	case 'b':
	  data += (nv * sizeof(uint8_t));
	  break;

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  //return json_rpc_error(-32603, rtn, allocator);
	  return NULL;

      }
      off++;
    }

    pch = strtok (NULL, " ");
  }

  free(tmp);
  return NULL;
}


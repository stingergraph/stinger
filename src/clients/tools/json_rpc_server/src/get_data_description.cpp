#include "json_rpc_server.h"
#include "json_rpc.h"
#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


static int
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
  (void) strtok_r (tmp, " ", &placeholder);

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
JSON_RPC_get_data_description::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
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



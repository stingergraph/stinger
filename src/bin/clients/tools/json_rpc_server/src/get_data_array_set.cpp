//#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

#include "rapidjson/document.h"
#include "json_rpc_server.h"
#include "json_rpc.h"

using namespace gt::stinger;


int64_t 
JSON_RPC_get_data_array_set::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
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


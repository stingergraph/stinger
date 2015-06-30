#include "json_rpc_server.h"
#include "json_rpc.h"
#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


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


#include "json_rpc_server.h"
#include "json_rpc.h"
#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


int64_t 
JSON_RPC_get_data_array_sorted_range::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
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
	LOG_E_A ("Algorithm is not running: %s", algorithm_name);
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


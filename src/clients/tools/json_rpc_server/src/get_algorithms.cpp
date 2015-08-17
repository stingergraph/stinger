#include "json_rpc_server.h"
#include "json_rpc.h"
#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


static int
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
JSON_RPC_get_algorithms::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  return algorithms_to_json(server_state, result, allocator);
}



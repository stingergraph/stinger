#include "json_rpc_server.h"
#include "json_rpc.h"
#include <sys/types.h>
#include <unistd.h>

#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

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


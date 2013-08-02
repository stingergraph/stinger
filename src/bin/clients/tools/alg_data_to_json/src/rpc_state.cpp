#include "rpc_state.h"

JSON_RPCServerState &
JSON_RPCServerState::get_server_state() {
  static JSON_RPCServerState state;
  return state;
}

JSON_RPCServerState::JSON_RPCServerState() : alg_lock(1) { }

size_t
JSON_RPCServerState::get_num_algs()
{
  size_t num = 0;
  readfe((uint64_t *)&alg_lock);
  num = algs.size();
  writeef((uint64_t *)&alg_lock, 1);
  return num;
}

StingerAlgState *
JSON_RPCServerState::get_alg(size_t num)
{
  StingerAlgState * rtn = NULL;
  readfe((uint64_t *)&alg_lock);
  rtn = algs[num];
  writeef((uint64_t *)&alg_lock, 1);
  return rtn;
}

StingerAlgState *
JSON_RPCServerState::get_alg(const std::string & name)
{
  StingerAlgState * rtn = NULL;
  readfe((uint64_t *)&alg_lock);
  rtn = alg_map[name];
  writeef((uint64_t *)&alg_lock, 1);
  return rtn;
}

size_t
JSON_RPCServerState::add_alg(StingerAlgState * alg)
{
  size_t rtn = 0;
  readfe((uint64_t *)&alg_lock);
  alg_map[alg->name] = alg;
  rtn = algs.size();
  algs.push_back(alg);
  writeef((uint64_t *)&alg_lock, 1);
  return rtn;
}

bool
JSON_RPCServerState::has_alg(const std::string & name)
{
  bool rtn = false;
  readfe((uint64_t *)&alg_lock);
  rtn = alg_map.count(name) > 0;
  writeef((uint64_t *)&alg_lock, 1);
  return rtn;
}

void
add_rpc_function(std::string name, JSON_RPCFunction * func)
{
  if(func)
    function_map[name] = func;
}

JSON_RPCFunction *
get_rpc_function(std::string name)
{
  return function_map[name];
}

bool
has_rpc_function(std::string name)
{
  return function_map.count(name) > 0;
}

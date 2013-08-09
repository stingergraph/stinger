#ifndef _JSON_RPC_SERVER_H
#define _JSON_RPC_SERVER_H

#include "rpc_state.h"

using namespace gt::stinger;


typedef enum {
  RANGE,
  SORTED,
  SET,
  ARRAY,
  STRIDE
} json_rpc_array_meth_t;

template <typename T>
struct compare_pair_desc {
  compare_pair_desc (T * vals) : _vals(vals) {}
  bool operator()(int64_t a, int64_t b) {
    if (_vals[a] > _vals[b])
      return true;
    else
      return false;
  }

private:
  T * _vals;
};

template <typename T>
struct compare_pair_asc {
  compare_pair_asc (T * vals) : _vals(vals) {}
  bool operator()(int64_t a, int64_t b) {
    if (_vals[a] < _vals[b])
      return true;
    else
      return false;
  }

private:
  T * _vals;
};


struct JSON_RPC_get_algorithms : JSON_RPCFunction {
  JSON_RPC_get_algorithms(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};

struct JSON_RPC_get_data_description : JSON_RPCFunction {
  JSON_RPC_get_data_description(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};

struct JSON_RPC_get_data_array : JSON_RPCFunction {
  JSON_RPC_get_data_array(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};

struct JSON_RPC_get_data_array_range : JSON_RPCFunction {
  JSON_RPC_get_data_array_range(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};

struct JSON_RPC_get_data_array_sorted_range : JSON_RPCFunction {
  JSON_RPC_get_data_array_sorted_range(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};

struct JSON_RPC_get_data_array_set : JSON_RPCFunction {
  JSON_RPC_get_data_array_set(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};

struct JSON_RPC_get_data_array_stride : JSON_RPCFunction {
  JSON_RPC_get_data_array_stride(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};


int
algorithms_to_json (JSON_RPCServerState * server_state,
  rapidjson::Value& rtn,
  rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator);

int
description_string_to_json (const char * description_string,
  rapidjson::Value& rtn,
  rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator);

int
array_to_json_monolithic   (json_rpc_array_meth_t method, stinger_t * S,
			    rapidjson::Value& rtn,
			    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator,
			    const char * description_string, int64_t nv, uint8_t * data,
			    bool strings,
			    const char * search_string,
			    int64_t start, int64_t end,
			    const char * order_str = NULL,
			    int64_t * set = NULL, int64_t set_len = 0,
			    int64_t stride = 1
			    );


#endif /* _JSON_RPC_SERVER_H */

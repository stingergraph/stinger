#ifndef _ALG_DATA_TO_JSON_H
#define _ALG_DATA_TO_JSON_H

#include "rpc_state.h"

using namespace gt::stinger;

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


struct JSON_RPC_get_algorithms : JSON_RPCFunction {
  JSON_RPC_get_algorithms(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value & params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};

struct JSON_RPC_get_data_description : JSON_RPCFunction {
  JSON_RPC_get_data_description(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value & params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};

struct JSON_RPC_get_data_array : JSON_RPCFunction {
  JSON_RPC_get_data_array(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value & params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};

struct JSON_RPC_get_data_array_range : JSON_RPCFunction {
  JSON_RPC_get_data_array_range(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value & params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};

struct JSON_RPC_get_data_array_sorted_range : JSON_RPCFunction {
  JSON_RPC_get_data_array_sorted_range(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value & params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};

struct JSON_RPC_get_data_array_set : JSON_RPCFunction {
  JSON_RPC_get_data_array_set(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
  virtual int64_t operator()(rapidjson::Value & params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
};


int
description_string_to_json (const char * description_string,
  rapidjson::Value& rtn,
  rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator);

int
array_to_json_range (const char * description_string, int64_t nv, uint8_t * data,
		     const char * search_string, int64_t start, int64_t end,
		     rapidjson::Value& rtn,
		     rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator);

int
array_to_json_sorted_range (const char * description_string, int64_t nv, uint8_t * data,
			    const char * search_string, int64_t start, int64_t end,
			    rapidjson::Value& rtn,
			    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator);

int
array_to_json_set (const char * description_string, int64_t nv, uint8_t * data,
		   const char * search_string, int64_t * set,
		   rapidjson::Value& rtn,
		   rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator);

int
array_to_json (const char * description_string, int64_t nv, uint8_t * data,
	       const char * search_string,
	       rapidjson::Value& val,
	       rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator);


#endif /* _ALG_DATA_TO_JSON_H */

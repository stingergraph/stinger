#ifndef _SESSION_HANDLING_H
#define _SESSION_HANDLING_H

#include <set>
#include <utility>

#include "rpc_state.h"
#include "alg_data_array.h"


namespace gt {
  namespace stinger {

    class JSON_RPC_community_subgraph: public JSON_RPCSession {
      private:
	rpc_params_t p[5];
	int64_t _source;
	char * _algorithm_name;
	char * _data_array_name;
	bool _strings;
	AlgDataArray * _data;
	std::set<int64_t> _vertices;

	std::set<std::pair<int64_t, int64_t> > _insertions;
	std::set<std::pair<int64_t, int64_t> > _deletions;

      public:
	JSON_RPC_community_subgraph(int64_t sess_id, JSON_RPCServerState * session) : JSON_RPCSession(sess_id, session) {
	  p[0] = ((rpc_params_t) {"name", TYPE_STRING, &_algorithm_name, false, 0});
	  p[1] = ((rpc_params_t) {"data", TYPE_STRING, &_data_array_name, false, 0});
	  p[2] = ((rpc_params_t) {"source", TYPE_INT64, &_source, false, 0});
	  p[3] = ((rpc_params_t) {"strings", TYPE_BOOL, &_strings, true, 0});
	  p[4] = ((rpc_params_t) {NULL, TYPE_NONE, NULL, false, 0});
	}
	virtual rpc_params_t * get_params();
	virtual int64_t update(StingerBatch & batch);
	virtual int64_t onRegister(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
	virtual int64_t onRequest(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
    };


  }
}

gt::stinger::AlgDataArray *
description_string_to_pointer (const char * description_string,
				uint8_t * data,
				int64_t nv,
				const char * search_string);



#endif /* _SESSION_HANDLING_H */

#ifndef _SESSION_HANDLING_H
#define _SESSION_HANDLING_H

#include <set>
#include <utility>

#include "rpc_state.h"
#include "alg_data_array.h"
#include "stinger_net/stinger_alg_state.h"

using namespace gt::stinger;

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
	  p[2] = ((rpc_params_t) {"source", TYPE_VERTEX, &_source, false, 0});
	  p[3] = ((rpc_params_t) {"strings", TYPE_BOOL, &_strings, true, 0});
	  p[4] = ((rpc_params_t) {NULL, TYPE_NONE, NULL, false, 0});
	}
	virtual rpc_params_t * get_params();
	virtual int64_t update(const StingerBatch & batch);
	virtual int64_t onRegister(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
	virtual int64_t onRequest(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
	virtual JSON_RPCSession * gimme(int64_t sess_id, JSON_RPCServerState * session) {
	  return new JSON_RPC_community_subgraph(sess_id, session);
	}
    };

    class JSON_RPC_vertex_event_notifier: public JSON_RPCSession {
      private:
	rpc_params_t p[3];
	params_array_t set_array;
	bool _strings;
	std::set<int64_t> _vertices;

	std::set<std::pair<int64_t, int64_t> > _insertions;
	std::set<std::pair<int64_t, int64_t> > _deletions;

      public:
	JSON_RPC_vertex_event_notifier(int64_t sess_id, JSON_RPCServerState * session) : JSON_RPCSession(sess_id, session) {
	  p[0] = ((rpc_params_t) {"set", TYPE_ARRAY, &set_array, false, 0});
	  p[1] = ((rpc_params_t) {"strings", TYPE_BOOL, &_strings, true, 0});
	  p[2] = ((rpc_params_t) {NULL, TYPE_NONE, NULL, false, 0});
	}
	virtual rpc_params_t * get_params();
	virtual int64_t update(const StingerBatch & batch);
	virtual int64_t onRegister(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
	virtual int64_t onRequest(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
	virtual JSON_RPCSession * gimme(int64_t sess_id, JSON_RPCServerState * session) {
	  return new JSON_RPC_vertex_event_notifier(sess_id, session);
	}
    };

    class JSON_RPC_get_latlon : public JSON_RPCSession {
      private:
	rpc_params_t p[2];
	int64_t _count;
	bool _strings;

	std::set<std::pair<double, double> > _coordinates;

      public:
	JSON_RPC_get_latlon(int64_t sess_id, JSON_RPCServerState * session) : JSON_RPCSession(sess_id, session) {
	  p[0] = ((rpc_params_t) {"count", TYPE_INT64, &_count, false, 0});
	  p[1] = ((rpc_params_t) {NULL, TYPE_NONE, NULL, false, 0});
	}
	virtual rpc_params_t * get_params();
	virtual int64_t update(const StingerBatch & batch);
	virtual int64_t onRegister(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
	virtual int64_t onRequest(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
	virtual JSON_RPCSession * gimme(int64_t sess_id, JSON_RPCServerState * session) {
	  return new JSON_RPC_get_latlon(sess_id, session);
	}
    };

    class JSON_RPC_get_latlon_gnip : public JSON_RPCSession {
      private:
	rpc_params_t p[2];
	int64_t _count;
	bool _strings;

	class semantic_coordinates {
	  public:
	    double _lat, _lon;
	    double _sentiment;
	    std::string _categories;

	    bool operator< (const semantic_coordinates& x) const {
	      if (_lat != x._lat)
		return _lat < x._lat;
	      else if (_lon != x._lon)
		return _lon < x._lon;
	      else
		return _sentiment < x._sentiment;
	    }

	    semantic_coordinates(double lat, double lon, double sent, std::string cat) {
	      _lat = lat;
	      _lon = lon;
	      _sentiment = sent;
	      _categories = cat;
	    }
	};

	std::set<semantic_coordinates> _coordinates;

      public:
	JSON_RPC_get_latlon_gnip(int64_t sess_id, JSON_RPCServerState * session) : JSON_RPCSession(sess_id, session) {
	  p[0] = ((rpc_params_t) {"count", TYPE_INT64, &_count, false, 0});
	  p[1] = ((rpc_params_t) {NULL, TYPE_NONE, NULL, false, 0});
	}
	virtual rpc_params_t * get_params();
	virtual int64_t update(const StingerBatch & batch);
	virtual int64_t onRegister(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
	virtual int64_t onRequest(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
	virtual JSON_RPCSession * gimme(int64_t sess_id, JSON_RPCServerState * session) {
	  return new JSON_RPC_get_latlon_gnip(sess_id, session);
	}
    };

    class JSON_RPC_get_latlon_twitter : public JSON_RPCSession {
      private:
	rpc_params_t p[2];
	int64_t _count;
	bool _strings;

	class semantic_coordinates {
	  public:
	    uint64_t _ref;
	    double _lat, _lon;
	    double _sentiment;
	    std::string _categories;

	    bool operator< (const semantic_coordinates& x) const {
	      if (_ref != x._ref)
		return _ref < x._ref;
	      else if (_lat != x._lat)
		return _lat < x._lat;
	      else if (_lon != x._lon)
		return _lon < x._lon;
	      else
		return _sentiment < x._sentiment;
	    }

	    semantic_coordinates(uint64_t ref, double lat, double lon, double sent, std::string cat) {
	      _ref = ref;
	      _lat = lat;
	      _lon = lon;
	      _sentiment = sent;
	      _categories = cat;
	    }
	};

	std::set<semantic_coordinates> _coordinates;

      public:
	JSON_RPC_get_latlon_twitter(int64_t sess_id, JSON_RPCServerState * session) : JSON_RPCSession(sess_id, session) {
	  p[0] = ((rpc_params_t) {"count", TYPE_INT64, &_count, false, 0});
	  p[1] = ((rpc_params_t) {NULL, TYPE_NONE, NULL, false, 0});
	}
	virtual rpc_params_t * get_params();
	virtual int64_t update(const StingerBatch & batch);
	virtual int64_t onRegister(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
	virtual int64_t onRequest(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
	virtual JSON_RPCSession * gimme(int64_t sess_id, JSON_RPCServerState * session) {
	  return new JSON_RPC_get_latlon_twitter(sess_id, session);
	}
    };


  }
}

gt::stinger::AlgDataArray *
description_string_to_pointer (gt::stinger::JSON_RPCServerState * server_state,
				const char * algorithm_name,
				const char * description_string,
				uint8_t * data,
				int64_t nv,
				const char * search_string);

#endif /* _SESSION_HANDLING_H */

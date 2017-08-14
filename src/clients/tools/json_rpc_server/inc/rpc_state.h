#ifndef RPC_STATE_H_
#define RPC_STATE_H_

#include <cstdlib>
#include <cstdio>
#include <map>
#include <string>
#include <stdint.h>
#include <semaphore.h>
#include "stinger_net/stinger_alg_state.h"
#include "stinger_net/stinger_local_state_c.h"
#include "stinger_core/stinger.h"
#include "rapidjson/document.h"
#include "stinger_net/proto/stinger-monitor.pb.h"
#include "stinger_net/stinger_mon.h"

namespace gt {
  namespace stinger {

    typedef enum {
      TYPE_VERTEX,
      TYPE_EDGE_TYPE,
      TYPE_VERTEX_TYPE,
      TYPE_INT64,
      TYPE_STRING,
      TYPE_DOUBLE,
      TYPE_ARRAY,
      TYPE_BOOL,
      TYPE_NONE
    } type_t;

    struct params_array_t {
      int64_t * arr;
      int64_t len;
      params_array_t();
      ~params_array_t();
    };

    typedef struct {
      const char * name;
      type_t type;
      void * output;
      bool optional;
      int64_t def;
    } rpc_params_t;

    class JSON_RPCServerState;

    struct JSON_RPCFunction {
      JSON_RPCServerState * server_state;

      JSON_RPCFunction(JSON_RPCServerState * state) : server_state(state) {
      }

      virtual int64_t operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
	LOG_W("This is a generic JSON_RPCFunction object and should not be called");
        return -1;
      }

      bool contains_params(rpc_params_t * p, rapidjson::Value * params);

    };

    class JSON_RPCSession {
      private:
	int64_t the_lock;
	int64_t session_id;
	int64_t last_touched;
      protected:
	JSON_RPCServerState * server_state;

      public:
	JSON_RPCSession(int64_t sess_id, JSON_RPCServerState * state) : the_lock(0), session_id(sess_id), server_state(state) { }
	virtual ~JSON_RPCSession() { }
	void lock();
	void unlock();
	virtual rpc_params_t * get_params() {
	  LOG_W("This is a generic JSON_RPCSession object and should not be called");
	  return NULL;
	};
	virtual int64_t update(const StingerBatch & batch) {
	  LOG_W("This is a generic JSON_RPCSession object and should not be called");
          return -1;
	}
	virtual int64_t onRegister(
				rapidjson::Value & result,
				rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
	  LOG_W("This is a generic JSON_RPCSession object and should not be called");
          return -1;
	}
	virtual int64_t onRequest(
				rapidjson::Value & result,
				rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
	  LOG_W("This is a generic JSON_RPCSession object and should not be called");
          return -1;
	}
	virtual JSON_RPCSession * gimme(int64_t sess_id, JSON_RPCServerState * state) {
	  LOG_W("This is a generic JSON_RPCSession object and should not be called");
	  return NULL;
	}
	bool is_timed_out();
	int64_t reset_timeout();
	int64_t get_session_id();
	int64_t get_time_since();
    };

    class JSON_RPCServerState : public StingerMon {
      private:
	/* I'm a singleton */
	JSON_RPCServerState();
	~JSON_RPCServerState();

	std::map<std::string, JSON_RPCFunction *> function_map;
	std::map<std::string, JSON_RPCSession *> session_map;
	std::map<int64_t, JSON_RPCSession *> active_session_map;
	int64_t session_lock;

	int64_t max_sessions;
	int64_t next_session_id;

	time_t start_time;

      public:
	static JSON_RPCServerState & get_server_state();

	void
	add_rpc_function(std::string name, JSON_RPCFunction * func);

	JSON_RPCFunction *
	get_rpc_function(std::string name);

	bool
	has_rpc_function(std::string name);

	std::map<std::string, JSON_RPCFunction *>::iterator
	rpc_function_begin();

	std::map<std::string, JSON_RPCFunction *>::iterator
	rpc_function_end();

	void
	add_rpc_session(std::string name, JSON_RPCSession * func);

	JSON_RPCSession *
	get_rpc_session(std::string name);

	bool
	has_rpc_session(std::string name);

	int64_t
	get_next_session();

	int64_t
	add_session(int64_t session_id, JSON_RPCSession * session);

	void
	update_algs(stinger_t * stinger_copy, std::string new_loc, int64_t new_sz, 
	  std::vector<StingerAlgState *> * new_algs, std::map<std::string, StingerAlgState *> * new_alg_map,
	  const StingerBatch & batch);

	int64_t
	destroy_session(int64_t session_id);

	int64_t
	get_num_sessions();

	JSON_RPCSession *
	get_session(int64_t session_id);

	time_t
	get_time_since_start();
    };

  }
}

#endif /* RPC_STATE_H_ */

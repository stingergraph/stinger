#ifndef RPC_STATE_H_
#define RPC_STATE_H_

#include <map>
#include <stdint.h>
#include "stinger_net/stinger_alg_state.h"
#include "stinger_core/stinger.h"
#include "stinger_core/stinger_error.h"
#include "rapidjson/document.h"

namespace gt {
  namespace stinger {

    typedef enum {
      TYPE_INT64,
      TYPE_STRING,
      TYPE_DOUBLE
    } type_t;

    typedef struct {
      char * name;
      type_t type;
      void * output;
    } rpc_params_t;

    class JSON_RPCServerState;

    struct JSON_RPCFunction {
      JSON_RPCServerState * server_state;

      JSON_RPCFunction(JSON_RPCServerState * state) : server_state(state) {
	LOG_W("This is a generic JSON_RPCFunction object and should not be called");
      }

      virtual int64_t operator()(rapidjson::Value & params, rapidjson::Value & result) {
	LOG_W("This is a generic JSON_RPCFunction object and should not be called");
      }

      bool contains_params(rpc_params_t * p, rapidjson::Value & params) {
	while(p->name) {
	  if(!params.HasMember(p->name)) {
	    return false;
	  }

	  switch(p->type) {
	    case TYPE_INT64: {
	      if(!params[p->name].IsInt64()) {
		return false;
	      }
	      *((int64_t *)p->output) = params[p->name].GetInt64();
	    } break;
	    case TYPE_STRING: {
	      if(!params[p->name].IsString()) {
		return false;
	      }
	      *((char **)p->output) = (char *) params[p->name].GetString();
	    } break;
	    case TYPE_DOUBLE: {
	      if(!params[p->name].IsDouble()) {
		return false;
	      }
	      *((double *)p->output) = params[p->name].GetDouble();
	    } break;
	  }
	}
	return true;
      }
    };

    struct JSON_RPC_get_algorithms : JSON_RPCFunction {
      JSON_RPC_get_algorithms(JSON_RPCServerState * state) : JSON_RPCFunction(state) { }
      virtual int64_t operator()(rapidjson::Value & params, rapidjson::Value & result) {
	/* TODO */
	
      }
    };

    class JSON_RPCServerState {
      private:
	/* I'm a singleton */
	JSON_RPCServerState();

	int64_t alg_lock;
	std::vector<StingerAlgState *> algs;                     
	std::map<std::string, StingerAlgState *> alg_map;        
	std::map<std::string, JSON_RPCFunction *> function_map;


      public:
	static JSON_RPCServerState & get_server_state();

	size_t
	get_num_algs();

	StingerAlgState *
	get_alg(size_t num);

	StingerAlgState *
	get_alg(const std::string & name);

	size_t
	add_alg(StingerAlgState * alg);

	bool
	has_alg(const std::string & name);

	void
	add_rpc_function(std::string name, JSON_RPCFunction * func);

	JSON_RPCFunction *
	get_rpc_function(std::string name);

	bool
	has_rpc_function(std::string name);
    };

  }
}

#endif /* RPC_STATE_H_ */

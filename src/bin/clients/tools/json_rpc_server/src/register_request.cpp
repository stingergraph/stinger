#include "json_rpc_server.h"
#include "json_rpc.h"
#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


int64_t 
JSON_RPC_register::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  char * type_name;
  rpc_params_t p[] = {
    {"type", TYPE_STRING, &type_name, false, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  LOG_D ("Checking for parameter \"type:\"");
  if (!contains_params(p, params)) {
    return json_rpc_error(-32602, result, allocator);
  }
  
  /* Does the session type exist */
  LOG_D_A ("Searching for session type: %s", type_name);
  if (server_state->has_rpc_session(type_name) == false) {
    return json_rpc_error (-32601, result, allocator);
  }

  LOG_D ("Get a session id and create a session");
  /* create a session id and a new session of the requested type */
  /* add the session to the server state */
  int64_t next_session_id = server_state->get_next_session();
  JSON_RPCSession * session = server_state->get_rpc_session(type_name)->gimme(next_session_id, server_state);

  rapidjson::Value session_id;
  session_id.SetInt64(next_session_id);
  result.AddMember("session_id", session_id, allocator);

  LOG_D ("Check parameters for the session type");

  rpc_params_t * session_params = session->get_params();
  /* if things don't go so well, we need to undo everything to this point */
  if (!contains_params(session_params, params)) {
    delete session;
    return json_rpc_error(-32602, result, allocator);
  }

  session->lock();  /* I wish I didn't have to do this */
  /* push the session onto the stack */
  int64_t rtn = server_state->add_session(next_session_id, session);
  if (rtn == -1) {
    delete session;
    return json_rpc_error(-32002, result, allocator);
  }

  LOG_D ("Call the onRegister method for the session");

  /* this will send back the edge list to the client */
  //session->lock();
  session->onRegister(result, allocator);
  session->unlock();

  LOG_D ("Return");

  return 0;
}


int64_t 
JSON_RPC_request::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  int64_t session_id;
  bool strings;
  rpc_params_t p[] = {
    {"session_id", TYPE_INT64, &session_id, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  LOG_D ("Checking for parameters");

  if (!contains_params(p, params)) {
    return json_rpc_error(-32602, result, allocator);
  }

  LOG_D_A ("Check if session id %ld is valid", session_id);
  JSON_RPCSession * session = server_state->get_session(session_id);

  if (!session) {
    return json_rpc_error(-32001, result, allocator);
  }

  rapidjson::Value json_session_id;
  json_session_id.SetInt64(session_id);
  result.AddMember("session_id", json_session_id, allocator);

  LOG_D ("Call the onRequest method for the session");

  /* this will send back the edge list to the client */
  session->lock();
  session->onRequest(result, allocator);
  session->reset_timeout();
  session->unlock();

  rapidjson::Value time_since;
  time_since.SetInt64(session->get_time_since());
  result.AddMember("time_since", time_since, allocator);

  LOG_D ("Return");

  return 0;
}


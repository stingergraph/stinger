#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <functional>

extern "C" {
#include "stinger_core/xmalloc.h"
}

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "json_rpc.h"
#include "rpc_state.h"

#define LOG_AT_W
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


void
json_rpc_process_request (rapidjson::Document& document, rapidjson::Document& response)
{
  JSON_RPCServerState & server_state = JSON_RPCServerState::get_server_state();
  rapidjson::Value is_null;
  is_null.SetNull();
  rapidjson::Value result;
  result.SetObject();
  response.SetObject();

  rapidjson::Document::AllocatorType& allocator = response.GetAllocator();

  LOG_D("In the json_rpc_process_request function.");

  /* Is the input a valid JSON object -- should also check when it's parsed */
  if (!document.IsObject()) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", is_null, allocator);
    return;
  }

  LOG_D("Request is a JSON object.");

  /* Does it have a jsonrpc field */
  if (!document.HasMember("jsonrpc")) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", is_null, allocator);
    return;
  }

  LOG_D("Request has a JSON-RPC version.");

  /* Is the jsonrpc field a string */
  if (!document["jsonrpc"].IsString()) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", is_null, allocator);
    return;
  }

  LOG_D("Request JSON-RPC version is a string.");

  /* Is the jsonrpc field equal to 2.0 */
  if (strcmp(document["jsonrpc"].GetString(), "2.0") != 0) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", is_null, allocator);
    return;
  }

  LOG_D("Request JSON-RPC version is valid.");

  /* Does it have an id field */
  /* TODO: notifications will change this */
  if (!document.HasMember("id")) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32000, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", is_null, allocator);
    return;
  }

  LOG_D("Request has an id field.");

  /* Is the id field a number or a string */
  /* Get the id field */
  /*int64_t id_int;*/
  /*const char * id_str;*/
  if (document["id"].IsInt64()) {
    /*id_int = document["id"].GetInt64();*/
  }
  else if (document["id"].IsString()) {
    /*id_str = (const char *) document["id"].GetString();*/
  }
  else {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", is_null, allocator);
    return;
  }
  rapidjson::Value& id = document["id"];

  LOG_D("Request id field is valid.");

  /* Does it have a method field */
  if (!document.HasMember("method")) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", id, allocator);
    return;
  }

  LOG_D("Request has a method field.");

  /* Is the method field a string */
  if (!document["method"].IsString()) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", id, allocator);
    return;
  }

  LOG_D("Method field is a string.");

  /* Parse the method field */
  rapidjson::Value& method = document["method"];
  const char * method_str = method.GetString();

  /* Does the method exist */
  if (server_state.has_rpc_function(method_str) == false) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32601, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", id, allocator);
    return;
  }

  LOG_D_A("Method %s exists.", method_str);

  /* Does the params field exist */
  rapidjson::Value * params = NULL;
  if (document.HasMember("params")) {

    /* Params is an array */
    params = &document["params"];
  }
  
  LOG_D_A("Parameters read (if applicable).", method_str);

  if(params && params->IsObject() && params->HasMember("wait_for_update") && (*params)["wait_for_update"].GetBool()) {
    server_state.wait_for_sync();
  }

  server_state.get_alg_read_lock();
  /* call the function */
  response.AddMember("jsonrpc", "2.0", allocator);
  if ((*server_state.get_rpc_function(method_str))(params, result, allocator)) {
    response.AddMember("error", result, allocator);
  }
  else {
    response.AddMember("result", result, allocator);
  }
  response.AddMember("id", id, allocator);
  server_state.release_alg_read_lock();

}


int32_t
json_rpc_error (int32_t error_code, rapidjson::Value& err_obj, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  rapidjson::Value code, message;
  code.SetInt(error_code);
  LOG_D ("Creating a JSON RPC error");

  switch (error_code) {
    case (-32700):
      {
	message.SetString("Parse error");
      } break;

    case (-32600):
      {
	message.SetString("Invalid request");
      } break;

    case (-32601):
      {
	message.SetString("Method not found");
      } break;

    case (-32602):
      {
	message.SetString("Invalid params");
      } break;

    case (-32603):
      {
	message.SetString("Internal error");
      } break;

    case (-32000):
      {
	message.SetString("Unimplemented");
      } break;
    
    case (-32001):
      {
	message.SetString("Invalid session");
      } break;
    
    case (-32002):
      {
	message.SetString("Too many sessions");
      } break;
    
    case (-32003):
      {
	message.SetString("Unknown algorithm");
      } break;
  }

  if ( (error_code <= -32004) && (error_code >= -32099)) {
    message.SetString("Server error");
  }

  err_obj.AddMember("code", code, allocator);
  err_obj.AddMember("message", message, allocator);

  return error_code;
}


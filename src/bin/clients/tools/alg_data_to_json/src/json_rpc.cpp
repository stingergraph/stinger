#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <functional>

extern "C" {
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
}

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "json_rpc.h"
#include "rpc_state.h"

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



  /* Is the input a valid JSON object -- should also check when it's parsed */
  if (!document.IsObject()) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", is_null, allocator);
    return;
  }

  /* Does it have a jsonrpc field */
  if (!document.HasMember("jsonrpc")) {

    response.AddMember("jsonrpc", "2.0", allocator);

    json_rpc_error (-32600, result, allocator);

    response.AddMember("error", result, allocator);

    response.AddMember("id", is_null, allocator);
    return;
  }


  /* Is the jsonrpc field a string */
  if (!document["jsonrpc"].IsString()) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", is_null, allocator);
    return;
  }

  /* Is the jsonrpc field equal to 2.0 */
  if (strcmp(document["jsonrpc"].GetString(), "2.0") != 0) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", is_null, allocator);
    return;
  }


  /* Does it have an id field */
  /* TODO: notifications will change this */
  if (!document.HasMember("id")) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32000, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", is_null, allocator);
    return;
  }



  /* Is the id field a number of a string */
  /* Get the id field */
  int64_t id_int;
  const char * id_str;
  if (document["id"].IsInt64()) {
    id_int = document["id"].GetInt64();
  }
  else if (document["id"].IsString()) {
    id_str = (const char *) document["id"].GetString();
  }
  else {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", is_null, allocator);
    return;
  }
  rapidjson::Value& id = document["id"];


  /* Does it have a method field */
  if (!document.HasMember("method")) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", id, allocator);
    return;
  }


  /* Is the method field a string */
  if (!document["method"].IsString()) {
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error (-32600, result, allocator);
    response.AddMember("error", result, allocator);
    response.AddMember("id", id, allocator);
    return;
  }


  /* Parse the method field */


  /* Does the method exist */
  //server_state.has_rpc_function(/*method name*/);

  /* Does the params field exist */
  if (document.HasMember("params")) {

    /* Params is an array */
    const rapidjson::Value& params = document["params"];
  }


  /* call the function */
  /*error_code <- (params, response)
  if (error_code)
    response.AddMember("jsonrpc", "2.0", allocator);
    json_rpc_error(response); 
    response.AddMember("error", result, allocator);
    response.AddMember("id", id, allocator); */

}

void
json_rpc_response (rapidjson::Document& document, rapidjson::Value& result, rapidjson::Value& id)
{
  rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

  document.AddMember("jsonrpc", "2.0", allocator);
  document.AddMember("result", result, allocator);
  document.AddMember("id", id, allocator);
}


int32_t
json_rpc_error (int32_t error_code, rapidjson::Value& err_obj, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  rapidjson::Value code, message;
  code.SetInt(error_code);

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
  }

  if ( (error_code <= -32001) && (error_code >= -32099)) {
    message.SetString("Server error");
  }

  err_obj.AddMember("code", code, allocator);
  err_obj.AddMember("message", message, allocator);

  return error_code;
}

#ifndef _JSON_RPC_H
#define _JSON_RPC_H


void
json_rpc_process_request (rapidjson::Document& document, rapidjson::Document& response);

void
json_rpc_response (rapidjson::Document& document, rapidjson::Value& result, rapidjson::Value& id);

int32_t
json_rpc_error (int32_t error_code, rapidjson::Value& err_obj, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator);

#endif /* _JSON_RPC_H */

#ifndef _JSON_RPC_H
#define _JSON_RPC_H


void
json_rpc_response (rapidjson::Document& document, rapidjson::Value& result, rapidjson::Value& id);

void
json_rpc_error (rapidjson::Document& document, int32_t error_code, rapidjson::Value& id);

#endif /* _JSON_RPC_H */

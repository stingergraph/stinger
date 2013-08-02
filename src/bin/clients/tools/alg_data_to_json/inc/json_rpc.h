#ifndef _JSON_RPC_H
#define _JSON_RPC_H


rapidjson::Document *
json_rpc_response (rapidjson::Value& result, rapidjson::Value& id);

rapidjson::Document *
json_rpc_error (int32_t error_code, rapidjson::Value& id);

#endif /* _JSON_RPC_H */

#include "rapidjson/document.h"

template<class Allocator>
rapidjson::Value * document_to_value(rapidjson::Document & document, Allocator & alloc) {
  rapidjson::Value * rtn = new rapidjson::Value();

  if(document.IsObject()) {

    rtn->SetObject();
    for (rapidjson::Value::MemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) {
      rtn->AddMember(itr->name, itr->value, alloc);
    }
    return rtn;

  } else if (document.IsArray()) {

    rtn->SetArray();
    for (int64_t i = 0; i < document.Size(); i++) {
      rtn->PushBack(document[i], alloc);
    }
    return rtn;

  } else {
    delete rtn;
    return NULL;
  }
}

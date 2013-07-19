#include <cstdio>
#include <string>
#include <fstream>
#include "rapidjson/document.h"

int main(int argc, char *argv[])
{
  std::string line, text;
  std::ifstream in(argv[1]);

  while (std::getline(in, line))
  {
    text += line + "\n";
  }

  const char * json = text.c_str();

  rapidjson::Document document;
  document.Parse<0>(json);

  assert(document.IsObject());

  assert(document.HasMember("nv"));
  assert(document["nv"].IsNumber());
  assert(document["nv"].IsInt());
  int64_t nv = document["nv"].GetInt();
  printf("nv = %ld\n", nv);

  assert(document.HasMember("ne"));
  assert(document["ne"].IsNumber());
  assert(document["ne"].IsInt());
  int64_t ne = document["ne"].GetInt();
  printf("ne = %ld\n", ne);

  assert(document.HasMember("vertices"));
  const rapidjson::Value& vertices = document["vertices"];
  assert(vertices.IsArray());
  for (rapidjson::SizeType i = 0; i < vertices.Size(); i++)
  {
    const rapidjson::Value& vertex = vertices[i];
    assert(vertex.IsObject());

    assert(vertex.HasMember("id"));
    assert(vertex["id"].IsNumber());
    assert(vertex["id"].IsInt());
    int64_t id = vertex["id"].GetInt();
    printf("%ld ", id);

    assert(vertex.HasMember("name"));
    assert(vertex["name"].IsString());
    printf("%s ", vertex["name"].GetString());

    assert(vertex.HasMember("weight"));
    assert(vertex["weight"].IsNumber());
    assert(vertex["weight"].IsInt());
    int64_t weight = vertex["weight"].GetInt();
    printf("%ld ", weight);

    assert(vertex.HasMember("type"));
    assert(vertex["type"].IsString());
    printf("%s\n", vertex["type"].GetString());

    /* do something with all of this */
  }

  assert(document.HasMember("edges"));
  const rapidjson::Value& edges = document["edges"];
  assert(edges.IsArray());
  for (rapidjson::SizeType i = 0; i < edges.Size(); i++)
  {
    const rapidjson::Value& edge = edges[i];
    assert(edge.IsObject());

    assert(edge.HasMember("id"));
    assert(edge["id"].IsNumber());
    assert(edge["id"].IsInt());
    int64_t id = edge["id"].GetInt();
    printf("%ld ", id);

    assert(edge.HasMember("source_id"));
    assert(edge["source_id"].IsNumber());
    assert(edge["source_id"].IsInt());
    int64_t source_id = edge["source_id"].GetInt();
    printf("%ld ", source_id);

    assert(edge.HasMember("source_name"));
    assert(edge["source_name"].IsString());
    printf("%s ", edge["source_name"].GetString());

    assert(edge.HasMember("target_id"));
    assert(edge["target_id"].IsNumber());
    assert(edge["target_id"].IsInt());
    int64_t target_id = edge["target_id"].GetInt();
    printf("%ld ", target_id);

    assert(edge.HasMember("target_name"));
    assert(edge["target_name"].IsString());
    printf("%s ", edge["target_name"].GetString());

    assert(edge.HasMember("weight"));
    assert(edge["weight"].IsNumber());
    assert(edge["weight"].IsInt());
    int64_t weight = edge["weight"].GetInt();
    printf("%ld ", weight);
    
    assert(edge.HasMember("time_first"));
    assert(edge["time_first"].IsNumber());
    assert(edge["time_first"].IsInt());
    int64_t time_first = edge["time_first"].GetInt();
    printf("%ld ", time_first);

    assert(edge.HasMember("time_last"));
    assert(edge["time_last"].IsNumber());
    assert(edge["time_last"].IsInt());
    int64_t time_last= edge["time_last"].GetInt();
    printf("%ld ", time_last);

    assert(edge.HasMember("type"));
    assert(edge["type"].IsString());
    printf("%s\n", edge["type"].GetString());


  }







  return 0;
}

#include <cstdio>
#include <string>
#include <fstream>
#include "rapidjson/document.h"

extern "C" {
#include "stinger_core/stinger.h"
}

extern "C" int
load_json_graph (struct stinger * S, const char * filename)
{
  std::string line, text;
  std::ifstream in(filename);

  while (std::getline(in, line))
  {
    text += line + "\n";
  }

  const char * json = text.c_str();

  rapidjson::Document document;
  document.Parse<0>(json);

  assert(document.IsObject());
/*
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
*/
  assert(document.HasMember("vertices"));
  const rapidjson::Value& vertices = document["vertices"];
  assert(vertices.IsArray());
  for (rapidjson::SizeType i = 0; i < vertices.Size(); i++)
  {
    int64_t id = -1;
    int64_t weight = 0;
    int64_t type = 0;

    const rapidjson::Value& vertex = vertices[i];
    assert(vertex.IsObject());

    /* if the file has a vertex id, we are going to use it */
    if (vertex.HasMember("id") && vertex["id"].IsNumber() && vertex["id"].IsInt())
      id = vertex["id"].GetInt();

    if (id < 0) /* file did not specify a vertex id */
    {
      if (vertex.HasMember("name") && vertex["name"].IsString())
	stinger_mapping_create (S, vertex["name"].GetString(), strlen(vertex["name"].GetString()), &id);
      else
	assert(id >= 0);
    }

    if (vertex.HasMember("weight") && vertex["weight"].IsNumber() && vertex["weight"].IsInt())
      weight = vertex["weight"].GetInt();

    if (vertex.HasMember("type") && vertex["type"].IsString())
    {
      type = stinger_vtype_names_lookup_type (S, (char *)vertex["type"].GetString());
      if (type == -1) {
	stinger_vtype_names_create_type (S, (char *)vertex["type"].GetString(), &type);
      }
      if (type == -1) {
	perror ("Failed to create new edge type");
	exit(-1);
      }
    }

    stinger_vweight_set (S, id, weight);
    stinger_vtype_set (S, id, type);
    /* done with this vertex */
  }

  /* now for the edges */
  assert(document.HasMember("edges"));
  const rapidjson::Value& edges = document["edges"];
  assert(edges.IsArray());
  for (rapidjson::SizeType i = 0; i < edges.Size(); i++)
  {
    int64_t source_id;
    int64_t target_id;
    int64_t weight = 0;
    int64_t ts = 0;
    int64_t type = 0;

    const rapidjson::Value& edge = edges[i];
    assert(edge.IsObject());
/*
    assert(edge.HasMember("id"));
    assert(edge["id"].IsNumber());
    assert(edge["id"].IsInt());
    int64_t id = edge["id"].GetInt();
    printf("%ld ", id);
*/  /* Maybe we'll use this with the kv_store at some point */

    if (edge.HasMember("source_id") && edge["source_id"].IsNumber() && edge["source_id"].IsInt())
    {
      source_id = edge["source_id"].GetInt();

      assert(edge.HasMember("target_id"));
      assert(edge["target_id"].IsNumber());
      assert(edge["target_id"].IsInt());
      target_id = edge["target_id"].GetInt();
    }
    else
    {
      assert(edge.HasMember("source_name"));
      assert(edge["source_name"].IsString());
      stinger_mapping_create (S, edge["source_name"].GetString(), strlen(edge["source_name"].GetString()), &source_id);

      assert(edge.HasMember("target_name"));
      assert(edge["target_name"].IsString());
      stinger_mapping_create (S, edge["target_name"].GetString(), strlen(edge["target_name"].GetString()), &target_id);
    }

    if (edge.HasMember("weight") && edge["weight"].IsNumber() && edge["weight"].IsInt())
      weight = edge["weight"].GetInt();
    
    if (edge.HasMember("time_first") && edge["time_first"].IsNumber() && edge["time_first"].IsInt())
      ts = edge["time_first"].GetInt();

    if (edge.HasMember("time_last") && edge["time_last"].IsNumber() && edge["time_last"].IsInt())
      ts = edge["time_last"].GetInt();

    if (edge.HasMember("type") && edge["type"].IsString())
    {
      type = stinger_etype_names_lookup_type (S, (char *)edge["type"].GetString());
      if (type == -1) {
	stinger_etype_names_create_type (S, (char *)edge["type"].GetString(), &type);
      }
      if (type == -1) {
	perror ("Failed to create new edge type");
	exit(-1);
      }
    }

    stinger_insert_edge (S, type, source_id, target_id, weight, ts);
    /* and we're done with this edge */
  }

  return 0;
}

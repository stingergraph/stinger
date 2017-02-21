#include <cstdlib>
#include <cstdio>
#include <set>

//#define LOG_AT_W  /* warning only */

#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"

#include "session_handling.h"
#include "json_rpc.h"
#include "alg_data_array.h"

#define LOG_AT_W
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


/* stateful subgraph extraction */

rpc_params_t *
JSON_RPC_community_subgraph::get_params()
{
  return p;
}


int64_t
JSON_RPC_community_subgraph::update(const StingerBatch & batch)
{
  if (0 == batch.insertions_size () && 0 == batch.deletions_size ()) { 
    return 0;
  }

  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return -1;
  }

  /* refresh the AlgDataArray object */
  _data->refresh();


  /* TODO: this does not take "undirected batch" into account */

  for(size_t d = 0; d < batch.deletions_size(); d++) {
    const EdgeDeletion & del = batch.deletions(d);

    int64_t src = del.source();
    int64_t dst = del.destination();

    /* both source and destination vertices were previously tracked,
       i.e. the edge that was deleted was previously on the screen */
    if ( _vertices.find(src) != _vertices.end() && _vertices.find(dst) != _vertices.end() ) {
      LOG_D_A("Adding <%ld, %ld> to deletions", (long) src, (long) dst);
      _deletions.insert(std::make_pair(src, dst));
    }
  }

  /* edge insertions whose endpoints are within the same community get sent
     to the client on the next request */
  for (size_t i = 0; i < batch.insertions_size(); i++) {
    const EdgeInsertion & in = batch.insertions(i);
    
    int64_t src = in.source();
    int64_t dst = in.destination();

    if (_data->equal(_source,src) && _data->equal(_source,dst)) {
      LOG_D_A("Adding <%ld, %ld> to insertions", (long) src, (long) dst);
      _insertions.insert(std::make_pair(src, dst));
    }
  }

  std::set<int64_t>::iterator it;

  for (it = _vertices.begin(); it != _vertices.end(); ++it) {
    LOG_D_A("Vertex %ld is in the vertices list", (long) *it);
    if (!(_data->equal(_source, *it))) {
      LOG_D("but is no longer in the group");

      STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, *it) {
        /* edge used to be in the community */
        if ( _vertices.find(STINGER_EDGE_DEST) != _vertices.end() ) {
          _deletions.insert(std::make_pair(*it, STINGER_EDGE_DEST));
        }
      } STINGER_FORALL_OUT_EDGES_OF_VTX_END();

      _vertices.erase(*it);  /* remove it from the vertex set */

    }
  }

  /* Add all vertices with the same label to the vertices[] set */
  for (int64_t i = 0; i < _data->length(); i++) {
    /* _source and i must be in the same community, and i must not be in the _vertices set */
    if (_data->equal(_source, i) && _vertices.find(i) == _vertices.end()) {
      LOG_D_A("Vertex %ld is in the community, but not in the vertices list", (long) i);
      _vertices.insert(i);

      STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, i) {
        /* if the edge is in the community */
        if (_data->equal(i, STINGER_EDGE_DEST)) {
          LOG_D_A("and it has an edge inside the community to %ld", (long) STINGER_EDGE_DEST);
          _insertions.insert(std::make_pair(i, STINGER_EDGE_DEST));
        }
      } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
    }
  }

  return 0;
}


int64_t
JSON_RPC_community_subgraph::onRequest(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  rapidjson::Value insertions, deletions, insertions_str, deletions_str;
  rapidjson::Value src, src_str, dst, dst_str, edge, edge_str;
  std::set<std::pair<int64_t, int64_t> >::iterator it;

  stinger_t * S = server_state->get_stinger();

  /* send insertions back */
  insertions.SetArray();
  insertions_str.SetArray();

  for (it = _insertions.begin(); it != _insertions.end(); ++it) {
    src.SetInt64((*it).first);
    dst.SetInt64((*it).second);
    edge.SetArray();
    edge.PushBack(src, allocator);
    edge.PushBack(dst, allocator);
    insertions.PushBack(edge, allocator);
    if(_strings) {
      char * physID;
      uint64_t len;
      if(-1 == stinger_mapping_physid_direct(S, (*it).first, &physID, &len)) {
        physID = (char *) "";
        len = 0;
      }
      src_str.SetString(physID, len, allocator);
      if(-1 == stinger_mapping_physid_direct(S, (*it).second, &physID, &len)) {
        physID = (char *) "";
        len = 0;
      }
      dst_str.SetString(physID, len, allocator);
      edge_str.SetArray();
      edge_str.PushBack(src_str, allocator);
      edge_str.PushBack(dst_str, allocator);
      insertions_str.PushBack(edge_str, allocator);
    }
  }

  result.AddMember("insertions", insertions, allocator);
  if(_strings) {
    result.AddMember("insertions_str", insertions_str, allocator);
  }

  /* send deletions back */

  deletions.SetArray();
  deletions_str.SetArray();


  for (it = _deletions.begin(); it != _deletions.end(); ++it) {
    src.SetInt64((*it).first);
    dst.SetInt64((*it).second);
    edge.SetArray();
    edge.PushBack(src, allocator);
    edge.PushBack(dst, allocator);
    deletions.PushBack(edge, allocator);
    if(_strings) {
      char * physID;
      uint64_t len;
      if(-1 == stinger_mapping_physid_direct(S, (*it).first, &physID, &len)) {
        physID = (char *) "";
        len = 0;
      }
      src_str.SetString(physID, len, allocator);
      if(-1 == stinger_mapping_physid_direct(S, (*it).second, &physID, &len)) {
        physID = (char *) "";
        len = 0;
      }
      dst_str.SetString(physID, len, allocator);
      edge_str.SetArray();
      edge_str.PushBack(src_str, allocator);
      edge_str.PushBack(dst_str, allocator);
      deletions_str.PushBack(edge_str, allocator);
    }
  }

  result.AddMember("deletions", deletions, allocator);
  if(_strings) {
    result.AddMember("deletions_str", deletions_str, allocator);
  }

  /* clear both and reset the clock */
  _insertions.clear();
  _deletions.clear();

  return 0;
}


int64_t
JSON_RPC_community_subgraph::onRegister(
		      rapidjson::Value & result,
		      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return json_rpc_error(-32603, result, allocator);
  }

  StingerAlgState * alg_state = server_state->get_alg(_algorithm_name);
  if (!alg_state) {
    LOG_E ("Algorithm is not running");
    return json_rpc_error(-32003, result, allocator);
  }

  const char * description_string = alg_state -> data_description.c_str();
  int64_t nv = S->max_nv;
  uint8_t * data = (uint8_t *) alg_state->data;
  const char * search_string = _data_array_name;

  _data = description_string_to_pointer (server_state, _algorithm_name, description_string, data, nv, search_string);

  if (_data->type() != 'l') {
    return json_rpc_error(-32602, result, allocator);
  }

  /* Get the community label of the source vertex */
  int64_t community_id = _data->get_int64(_source);

  /* Add all vertices with the same label to the vertices[] set */
  for (int64_t i = 0; i < _data->length(); i++) {
    if (_data->equal(_source, i)) {
      LOG_D_A ("Inserting %ld into _vertices", (long) i);
      _vertices.insert(i);
    }
  }

  rapidjson::Value a (rapidjson::kArrayType);
  rapidjson::Value a_str (rapidjson::kArrayType);
  rapidjson::Value src, src_str, dst, dst_str, edge, edge_str;

  /* Get all edges within the community */
  std::set<int64_t>::iterator it;
  for (it = _vertices.begin(); it != _vertices.end(); ++it) {
    STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN (S, *it) {
      if (_vertices.find(STINGER_EDGE_DEST) != _vertices.end()) {
        // edge is in the community
        src.SetInt64(*it);
        dst.SetInt64(STINGER_EDGE_DEST);
        edge.SetArray();
        edge.PushBack(src, allocator);
        edge.PushBack(dst, allocator);
        a.PushBack(edge, allocator);
        if(_strings) {
          char * physID;
          uint64_t len;
          if(-1 == stinger_mapping_physid_direct(S, (*it), &physID, &len)) {
            physID = (char *) "";
            len = 0;
          }
          src_str.SetString(physID, len, allocator);
          if(-1 == stinger_mapping_physid_direct(S, STINGER_EDGE_DEST, &physID, &len)) {
            physID = (char *) "";
            len = 0;
          }
          dst_str.SetString(physID, len, allocator);
          edge_str.SetArray();
          edge_str.PushBack(src_str, allocator);
          edge_str.PushBack(dst_str, allocator);
          a_str.PushBack(edge_str, allocator);
        }
      }
    } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
  }

  result.AddMember("subgraph", a, allocator);
  if(_strings) {
    result.AddMember("subgraph_str", a_str, allocator);
  }

  reset_timeout();

  return 0;
}


AlgDataArray *
description_string_to_pointer (gt::stinger::JSON_RPCServerState * server_state,
				const char * algorithm_name,
				const char * description_string,
				uint8_t * data,
				int64_t nv,
				const char * search_string)
{
  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  /* the description string is space-delimited */
  char * ptr = strtok (tmp, " ");

  /* skip the formatting */
  char * pch = strtok (NULL, " ");

  while (pch != NULL)
  {
    if (strcmp(pch, search_string) == 0) {
      AlgDataArray * rtn = new AlgDataArray(server_state, algorithm_name, data, description_string[off], nv);
      free (tmp);
      return rtn;

    } else {
      switch (description_string[off]) {
	case 'f':
	  data += (nv * sizeof(float));
	  break;

	case 'd':
	  data += (nv * sizeof(double));
	  break;

	case 'i':
	  data += (nv * sizeof(int32_t));
	  break;

	case 'l':
	  data += (nv * sizeof(int64_t));
	  break;

	case 'b':
	  data += (nv * sizeof(uint8_t));
	  break;

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  //return json_rpc_error(-32603, rtn, allocator);
	  return NULL;

      }
      off++;
    }

    pch = strtok (NULL, " ");
  }

  free(tmp);
  return NULL;
}

/* vertex event notifier */
rpc_params_t *
JSON_RPC_vertex_event_notifier::get_params()
{
  return p;
}

int64_t
JSON_RPC_vertex_event_notifier::update(const StingerBatch & batch)
{
  if (0 == batch.insertions_size () && 0 == batch.deletions_size ()) { 
    return 0;
  }

  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return -1;
  }

  /* TODO: this does not take "undirected batch" into account */

  for(size_t d = 0; d < batch.deletions_size(); d++) {
    const EdgeDeletion & del = batch.deletions(d);

    int64_t src = del.source();
    int64_t dst = del.destination();

    /* either the source or the destination vertex is part of the subgraph,
       i.e. the edge that was deleted was previously on the screen */
    if ( _vertices.find(src) != _vertices.end() || _vertices.find(dst) != _vertices.end() ) {
      LOG_D_A("Adding <%ld, %ld> to deletions", (long) src, (long) dst);
      _deletions.insert(std::make_pair(src, dst));
    }
  }

  for(size_t d = 0; d < batch.insertions_size(); d++) {
    const EdgeInsertion & in = batch.insertions(d);

    int64_t src = in.source();
    int64_t dst = in.destination();

    /* either the source or the destination vertex is part of the subgraph,
       i.e. the edge should be sent to the client */
    if ( _vertices.find(src) != _vertices.end() || _vertices.find(dst) != _vertices.end() ) {
      LOG_D_A("Adding <%ld, %ld> to insertions", (long) src, (long) dst);
      _insertions.insert(std::make_pair(src, dst));
    }
  }

  return 0;
}

int64_t
JSON_RPC_vertex_event_notifier::onRegister(
	      rapidjson::Value & result,
	      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return json_rpc_error(-32603, result, allocator);
  }

  /* Add each vertex in the array to the vertices[] list */
  int64_t * set = set_array.arr;
  int64_t set_len = set_array.len;

  for (int64_t i = 0; i < set_len; i++) {
    int64_t vtx = set[i];
    _vertices.insert(vtx);
  }

  /* Send back all edges incident on those vertices */
  rapidjson::Value a (rapidjson::kArrayType);
  rapidjson::Value a_str (rapidjson::kArrayType);
  rapidjson::Value src, src_str, dst, dst_str, edge, edge_str;

  std::set<int64_t>::iterator it;
  for (it = _vertices.begin(); it != _vertices.end(); ++it) {
    STINGER_FORALL_EDGES_OF_VTX_BEGIN (S, *it) {
      int64_t src_id, dst_id;
      if (STINGER_IS_OUT_EDGE) {
        src_id = (*it);
        dst_id = STINGER_EDGE_DEST;

        src.SetInt64(src_id);
        dst.SetInt64(dst_id);
        edge.SetArray();
        edge.PushBack(src, allocator);
        edge.PushBack(dst, allocator);
        a.PushBack(edge, allocator);
        if(_strings) {
          char * physID;
          uint64_t len;
          if(-1 == stinger_mapping_physid_direct(S, src_id, &physID, &len)) {
            physID = (char *) "";
            len = 0;
          }
          src_str.SetString(physID, len, allocator);
          if(-1 == stinger_mapping_physid_direct(S, dst_id, &physID, &len)) {
            physID = (char *) "";
            len = 0;
          }
          dst_str.SetString(physID, len, allocator);
          edge_str.SetArray();
          edge_str.PushBack(src_str, allocator);
          edge_str.PushBack(dst_str, allocator);
          a_str.PushBack(edge_str, allocator);
        }
      }
      if (STINGER_IS_IN_EDGE) {
        src_id = STINGER_EDGE_DEST;
        dst_id = (*it);

        src.SetInt64(src_id);
        dst.SetInt64(dst_id);
        edge.SetArray();
        edge.PushBack(src, allocator);
        edge.PushBack(dst, allocator);
        a.PushBack(edge, allocator);
        if(_strings) {
          char * physID;
          uint64_t len;
          if(-1 == stinger_mapping_physid_direct(S, src_id, &physID, &len)) {
            physID = (char *) "";
            len = 0;
          }
          src_str.SetString(physID, len, allocator);
          if(-1 == stinger_mapping_physid_direct(S, dst_id, &physID, &len)) {
            physID = (char *) "";
            len = 0;
          }
          dst_str.SetString(physID, len, allocator);
          edge_str.SetArray();
          edge_str.PushBack(src_str, allocator);
          edge_str.PushBack(dst_str, allocator);
          a_str.PushBack(edge_str, allocator);
        }
      }
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  result.AddMember("subgraph", a, allocator);
  if(_strings) {
    result.AddMember("subgraph_str", a_str, allocator);
  }

  reset_timeout();

  return 0;
}

int64_t
JSON_RPC_vertex_event_notifier::onRequest(
	      rapidjson::Value & result,
	      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  stinger_t * S = server_state->get_stinger();
  rapidjson::Value insertions, deletions, insertions_str, deletions_str;
  rapidjson::Value src, src_str, dst, dst_str, edge, edge_str;
  std::set<std::pair<int64_t, int64_t> >::iterator it;

  /* send insertions back */
  insertions.SetArray();
  insertions_str.SetArray();

  for (it = _insertions.begin(); it != _insertions.end(); ++it) {
    src.SetInt64((*it).first);
    dst.SetInt64((*it).second);
    edge.SetArray();
    edge.PushBack(src, allocator);
    edge.PushBack(dst, allocator);
    insertions.PushBack(edge, allocator);
    if(_strings) {
      char * physID;
      uint64_t len;
      if(-1 == stinger_mapping_physid_direct(S, (*it).first, &physID, &len)) {
        physID = (char *) "";
        len = 0;
      }
      src_str.SetString(physID, len, allocator);
      if(-1 == stinger_mapping_physid_direct(S, (*it).second, &physID, &len)) {
        physID = (char *) "";
        len = 0;
      }
      dst_str.SetString(physID, len, allocator);
      edge_str.SetArray();
      edge_str.PushBack(src_str, allocator);
      edge_str.PushBack(dst_str, allocator);
      insertions_str.PushBack(edge_str, allocator);
    }
  }

  result.AddMember("insertions", insertions, allocator);
  if(_strings) {
    result.AddMember("insertions_str", insertions_str, allocator);
  }

  /* send deletions back */

  deletions.SetArray();
  deletions_str.SetArray();

  for (it = _deletions.begin(); it != _deletions.end(); ++it) {
    src.SetInt64((*it).first);
    dst.SetInt64((*it).second);
    edge.SetArray();
    edge.PushBack(src, allocator);
    edge.PushBack(dst, allocator);
    deletions.PushBack(edge, allocator);
    if(_strings) {
      char * physID;
      uint64_t len;
      if(-1 == stinger_mapping_physid_direct(S, (*it).first, &physID, &len)) {
        physID = (char *) "";
        len = 0;
      }
      src_str.SetString(physID, len, allocator);
      if(-1 == stinger_mapping_physid_direct(S, (*it).second, &physID, &len)) {
        physID = (char *) "";
        len = 0;
      }
      dst_str.SetString(physID, len, allocator);
      edge_str.SetArray();
      edge_str.PushBack(src_str, allocator);
      edge_str.PushBack(dst_str, allocator);
      deletions_str.PushBack(edge_str, allocator);
    }
  }

  result.AddMember("deletions", deletions, allocator);
  if(_strings) {
    result.AddMember("deletions_str", deletions_str, allocator);
  }

  /* clear both and reset the clock */
  _insertions.clear();
  _deletions.clear();

  return 0;
}

/* latlon */
rpc_params_t *
JSON_RPC_get_latlon::get_params()
{
  return p;
}

int64_t
JSON_RPC_get_latlon::update(const StingerBatch & batch)
{
  if (0 == batch.insertions_size () && 0 == batch.deletions_size ()) { 
    return 0;
  }

  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return -1;
  }

  int sz = batch.metadata_size();

  for (int i = 0; i < sz; i++) {
    rapidjson::Document document;
    std::string metadata = batch.metadata(i);
    document.Parse<0>(metadata.c_str());

    if (!document.IsObject()) {
      continue;
    }
    
    if (!document.HasMember("geo")) {
      continue;
    }
  
    const rapidjson::Value& geo = document["geo"];

    if (!geo.IsObject()) {
      continue;
    }
    
    if (!geo.HasMember("coordinates")) {
      continue;
    }
  
    const rapidjson::Value& coord = geo["coordinates"];

    if (!coord.IsArray()) {
      continue;
    }

    double lat = coord[(rapidjson::SizeType) 0].GetDouble();
    double lon = coord[(rapidjson::SizeType) 1].GetDouble();

    //LOG_D_A ("Lat: %f, Lon: %f", lat, lon);
    _coordinates.insert(std::make_pair(lat, lon));
  }

  return 0;
}

int64_t
JSON_RPC_get_latlon::onRegister(
	      rapidjson::Value & result,
	      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return json_rpc_error(-32603, result, allocator);
  }


  reset_timeout();

  return 0;
}

int64_t
JSON_RPC_get_latlon::onRequest(
	      rapidjson::Value & result,
	      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  stinger_t * S = server_state->get_stinger();
  rapidjson::Value coord, lat, lon, pair;
  std::set<std::pair<double, double> >::iterator it;

  /* send insertions back */
  coord.SetArray();

  for (it = _coordinates.begin(); it != _coordinates.end(); ++it) {
    lat.SetDouble((*it).first);
    lon.SetDouble((*it).second);
    pair.SetObject();
    pair.AddMember("lat", lat, allocator);
    pair.AddMember("lon", lon, allocator);
    
    coord.PushBack(pair, allocator);
  }

  result.AddMember("coord", coord, allocator);

  /* clear both and reset the clock */
  _coordinates.clear();

  return 0;
}


/* gnip latlon + sentiment + categories */
rpc_params_t *
JSON_RPC_get_latlon_gnip::get_params()
{
  return p;
}

int64_t
JSON_RPC_get_latlon_gnip::update(const StingerBatch & batch)
{
  if (0 == batch.insertions_size () && 0 == batch.deletions_size ()) { 
    return 0;
  }

  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return -1;
  }

  int sz = batch.metadata_size();

  for (int i = 0; i < sz; i++) {
    rapidjson::Document document;
    std::string metadata = batch.metadata(i);
    document.Parse<0>(metadata.c_str());

    if (!document.IsObject()) {
      continue;
    }
    
    if (!document.HasMember("location")) {
      continue;
    }
  
    const rapidjson::Value& location = document["location"];

    if (!location.IsObject()) {
      continue;
    }
    
    if (!location.HasMember("lat")) {
      continue;
    }
  
    if (!location.HasMember("lng")) {
      continue;
    }
  
    const rapidjson::Value& obj_lat = location["lat"];
    const rapidjson::Value& obj_lng = location["lng"];

    double lat = obj_lat.GetDouble();
    double lon = obj_lng.GetDouble();

    //LOG_D_A ("Lat: %f, Lon: %f", lat, lon);
   
    double sentiment = 0;
    if (document.HasMember("sent") && document["sent"].IsDouble()) {
      sentiment = document["sent"].GetDouble();
    }
 
    std::string categories;
    if (document.HasMember("categories") && document["categories"].IsArray()) {
      const rapidjson::Value& a = document["categories"];
      for (rapidjson::SizeType i = 0; i < a.Size(); i++) {
	categories += a[i].GetString();
	categories += ",";
      }
    }
  
    semantic_coordinates tmp (lat, lon, sentiment, categories);
    _coordinates.insert(tmp);
  }

  return 0;
}

int64_t
JSON_RPC_get_latlon_gnip::onRegister(
	      rapidjson::Value & result,
	      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return json_rpc_error(-32603, result, allocator);
  }


  reset_timeout();

  return 0;
}

int64_t
JSON_RPC_get_latlon_gnip::onRequest(
	      rapidjson::Value & result,
	      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  stinger_t * S = server_state->get_stinger();
  rapidjson::Value coord, lat, lon, sentiment, categories, pair;
  std::set<semantic_coordinates>::iterator it;

  /* send insertions back */
  coord.SetArray();

  for (it = _coordinates.begin(); it != _coordinates.end(); ++it) {
    lat.SetDouble((*it)._lat);
    lon.SetDouble((*it)._lon);
    sentiment.SetDouble((*it)._sentiment);
    categories.SetString((*it)._categories.c_str(), (*it)._categories.length(), allocator);
    pair.SetObject();
    pair.AddMember("lat", lat, allocator);
    pair.AddMember("lon", lon, allocator);
    pair.AddMember("sentiment", sentiment, allocator);
    pair.AddMember("categories", categories, allocator);
    
    coord.PushBack(pair, allocator);
  }

  result.AddMember("coord", coord, allocator);

  /* clear both and reset the clock */
  _coordinates.clear();

  return 0;
}


/* Twitter sample stream latlon + sentiment + categories */
rpc_params_t *
JSON_RPC_get_latlon_twitter::get_params()
{
  return p;
}

int64_t
JSON_RPC_get_latlon_twitter::update(const StingerBatch & batch)
{
  if (0 == batch.insertions_size () && 0 == batch.deletions_size ()) { 
    return 0;
  }

  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return -1;
  }

  int sz = batch.metadata_size();

  for (int i = 0; i < sz; i++) {
    rapidjson::Document document;
    std::string metadata = batch.metadata(i);
    document.Parse<0>(metadata.c_str());

    if (!document.IsObject()) {
      continue;
    }
    
    if (!document.HasMember("geo")) {
      continue;
    }
  
    const rapidjson::Value& geo = document["geo"];

    if (!geo.IsObject()) {
      continue;
    }

    if (!geo.HasMember("coordinates")) {
      continue;
    }

    const rapidjson::Value& coord = geo["coordinates"];

    if (!coord.IsArray()) {
      continue;
    }
    
    double lat = coord[(rapidjson::SizeType) 0].GetDouble();
    double lon = coord[(rapidjson::SizeType) 1].GetDouble();

    //LOG_D_A ("Lat: %f, Lon: %f", lat, lon);
    uint64_t ref = 0;
    if (document.HasMember("id") && document["id"].IsInt64()) {
      ref = document["id"].GetInt64();
    }
   
    double sentiment = 0;
    if (document.HasMember("sent") && document["sent"].IsDouble()) {
      sentiment = document["sent"].GetDouble();
    }
 
    std::string categories;
    if (document.HasMember("categories") && document["categories"].IsArray()) {
      const rapidjson::Value& a = document["categories"];
      for (rapidjson::SizeType i = 0; i < a.Size(); i++) {
        categories += a[i].GetString();
        categories += ",";
      }
    }
  
    semantic_coordinates tmp (ref, lat, lon, sentiment, categories);
    _coordinates.insert(tmp);
  }

  return 0;
}

int64_t
JSON_RPC_get_latlon_twitter::onRegister(
	      rapidjson::Value & result,
	      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return json_rpc_error(-32603, result, allocator);
  }


  reset_timeout();

  return 0;
}

int64_t
JSON_RPC_get_latlon_twitter::onRequest(
	      rapidjson::Value & result,
	      rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  stinger_t * S = server_state->get_stinger();
  rapidjson::Value coord, ref, lat, lon, sentiment, categories, pair;
  std::set<semantic_coordinates>::iterator it;

  /* send insertions back */
  coord.SetArray();

  for (it = _coordinates.begin(); it != _coordinates.end(); ++it) {
    ref.SetInt64((*it)._ref);
    lat.SetDouble((*it)._lat);
    lon.SetDouble((*it)._lon);
    sentiment.SetDouble((*it)._sentiment);
    categories.SetString((*it)._categories.c_str(), (*it)._categories.length(), allocator);
    pair.SetObject();
    pair.AddMember("ref", ref, allocator);
    pair.AddMember("lat", lat, allocator);
    pair.AddMember("lon", lon, allocator);
    pair.AddMember("sentiment", sentiment, allocator);
    pair.AddMember("categories", categories, allocator);
    
    coord.PushBack(pair, allocator);
  }

  result.AddMember("coord", coord, allocator);

  /* clear both and reset the clock */
  _coordinates.clear();

  return 0;
}

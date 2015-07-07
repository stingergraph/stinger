#include "stinger_local_state_c.h"

extern "C" {
  #include <string.h>

  char stinger_description_str [] = "sssss vertex_outdegree vertex_indegree vertex_weight vertex_type_num vertex_type_name";

  char *
  stinger_local_state_get_data_description(stinger_t * S) {
    return stinger_description_str;
  }

  void *
  stinger_local_state_get_data_ptr(stinger_t * S, const char * data_name) {
    MAP_STING(S);

    if(0 == strncmp(data_name, "vertex_outdegree",16)) {
      return (void*)&(vertices->vertices[0].outDegree);
    } else if(0 == strncmp(data_name, "vertex_indegree",15)) {
      return (void*)&(vertices->vertices[0].inDegree);
    } else if(0 == strncmp(data_name, "vertex_weight",13)) {
      return (void*)&(vertices->vertices[0].weight);
    } else if(0 == strncmp(data_name, "vertex_type_num",15)) {
      return (void*)&(vertices->vertices[0].type);
    } else if(0 == strncmp(data_name, "vertex_type_name",16)) {
      return (void*)&(vertices->vertices[0].type);
    } else {
      return NULL;
    }
  }

  int64_t
  stinger_local_state_get_data(stinger_t * S, const char * data_name, int64_t vtx) {
    MAP_STING(S);

    if(0 == strncmp(data_name, "vertex_outdegree",16)) {
      return stinger_outdegree(S, vtx);
    } else if(0 == strncmp(data_name, "vertex_indegree",15)) {
      return stinger_indegree(S, vtx);
    } else if(0 == strncmp(data_name, "vertex_weight",13)) {
      return stinger_vweight_get(S, vtx);
    } else if(0 == strncmp(data_name, "vertex_type_num",15)) {
      return stinger_vtype_get(S, vtx);
    } else if(0 == strncmp(data_name, "vertex_type_name",16)) {
      return stinger_vtype_get(S, vtx);
    } else {
      return 0;
    }
  }

}
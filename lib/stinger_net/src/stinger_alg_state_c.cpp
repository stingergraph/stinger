#include "stinger_alg_state_c.h"
#include "stinger_alg_state.h"

using namespace gt::stinger;

extern "C" {
  const char *
  stinger_alg_state_get_name(void * self) {
    return ((StingerAlgState *)self)->name.c_str();
  }

  const char *
  stinger_alg_state_get_data_description(void * self) {
    return ((StingerAlgState *)self)->data_description.c_str();
  }

  const void *
  stinger_alg_state_get_data_ptr(void * self) {
    return ((StingerAlgState *)self)->data;
  }

  int64_t
  stinger_alg_state_data_per_vertex(void * self) {
    return ((StingerAlgState *)self)->data_per_vertex;
  }

  int64_t
  stinger_alg_state_level(void * self) {
    return ((StingerAlgState *)self)->level;
  }

  int64_t
  stinger_alg_state_number_dependencies(void * self) {
    return ((StingerAlgState *)self)->req_dep.size();
  }

  const char *
  stinger_alg_state_depencency(void * self, int64_t i) {
    return ((StingerAlgState *)self)->req_dep[i].c_str();
  }

}


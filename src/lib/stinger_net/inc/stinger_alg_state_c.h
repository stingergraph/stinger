#ifndef  STINGER_ALG_STATE_C_H
#define  STINGER_ALG_STATE_C_H

#include <stdint.h>

extern "C" {
  const char *
  stinger_alg_state_get_name(void * self);

  const char *
  stinger_alg_state_get_data_description(void * self);

  const void *
  stinger_alg_state_get_data_ptr(void * self);

  int64_t
  stinger_alg_state_data_per_vertex(void * self);

  int64_t
  stinger_alg_state_level(void * self);

  int64_t
  stinger_alg_state_number_dependencies(void * self);

  const char *
  stinger_alg_state_depencency(void * self, int64_t i);

}

#endif  /*STINGER_ALG_STATE_C_H*/

#ifndef  STINGER_LOCAL_STATE_C_H
#define  STINGER_LOCAL_STATE_C_H

extern "C" {
  #include "stinger_core/stinger.h"
  #include <stdint.h>

  char *
  stinger_local_state_get_data_description(stinger_t * S);

  void *
  stinger_local_state_get_data_ptr(stinger_t * S, const char * data_name);

  int64_t
  stinger_local_state_get_data(stinger_t * S, const char * data_name, int64_t vtx);

}

#endif
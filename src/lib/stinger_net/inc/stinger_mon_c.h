#ifndef  STINGER_MON_C_H
#define  STINGER_MON_C_H

#include <stdint.h>

#include "stinger_mon.h"

extern "C" {
  void *
  get_stinger_mon();

  size_t
  stinger_mon_num_algs(void * self);

  void *
  stinger_mon_get_alg_state(void * self, int64_t i);

  void *
  stinger_mon_get_alg_state_by_name(void * self, char * name);

  int
  stinger_mon_has_alg(void * self, char * name);

  void
  stinger_mon_get_read_lock(void * self);

  void
  stinger_mon_release_read_lock(void * self);

  void *
  stinger_mon_get_stinger(void * self);

  void
  stinger_mon_wait_for_sync(void * self);

  int64_t
  stinger_mon_get_max_nv();

}

#endif  /*STINGER_MON_C_H*/

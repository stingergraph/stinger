#include "stinger_mon.h"
#include "stinger_mon_c.h"

using namespace gt::stinger;

extern "C" {
  void *
  get_stinger_mon() {
    return (void *)&StingerMon::get_mon();
  }

  size_t
  stinger_mon_num_algs(void * self) {
    return ((StingerMon *)self)->get_num_algs();
  }

  void *
  stinger_mon_get_alg_state(void * self, int64_t i) {
    return (void *)((StingerMon *)self)->get_alg(i);
  }

  void *
  stinger_mon_get_alg_state_by_name(void * self, char * name) {
    return (void *)((StingerMon *)self)->get_alg(name);
  }

  int
  stinger_mon_has_alg(void * self, char * name) {
    return ((StingerMon *)self)->has_alg(name);
  }

  void
  stinger_mon_get_read_lock(void * self) {
    ((StingerMon *)self)->get_alg_read_lock();
  }

  void
  stinger_mon_release_read_lock(void * self) {
    ((StingerMon *)self)->release_alg_read_lock();
  }

  void *
  stinger_mon_get_stinger(void * self) {
    return ((StingerMon *)self)->get_stinger();
  }

  void
  stinger_mon_wait_for_sync(void * self) {
    ((StingerMon *)self)->wait_for_sync();
  }

  int64_t
  stinger_mon_get_max_nv() {
    return STINGER_MAX_LVERTICES;
  }
}

#include <pthread.h>
#include <sys/time.h>

#include "stinger_core/xmalloc.h"
#include "stinger_core/x86_full_empty.h"
#include "stinger_core/stinger_shared.h"
#include "stinger_core/stinger.h"
#include "stinger_mon.h"

#define LOG_AT_W
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;

static uint64_t singleton_lock = 0;
static StingerMon * state = NULL;

StingerMon &
StingerMon::get_mon() {
  if(!state) {
    readfe(&singleton_lock);
      if(!state) {
	state = new StingerMon();
      }
    writeef(&singleton_lock, 1);
  }
  return *state;
}

StingerMon::StingerMon() : stinger(NULL), 
  stinger_loc(""), stinger_sz(0), algs(NULL), alg_map(NULL),
  waiting(0), wait_lock(0), max_time(-922337203685477580)
{
  pthread_rwlock_init(&alg_lock, NULL);
  sem_init(&sync_lock, 0, 0);
}

StingerMon::~StingerMon()
{
  sem_destroy(&sync_lock);
}

size_t
StingerMon::get_num_algs()
{
  if(algs)
    return algs->size();
  else
    return 0;
}

StingerAlgState *
StingerMon::get_alg(size_t num)
{
  if(algs)
    return (*algs)[num];
  else
    return NULL;
}

StingerAlgState *
StingerMon::get_alg(const std::string & name)
{
  if(alg_map && has_alg(name))
    return (*alg_map)[name];
  else
    return NULL;
}

bool
StingerMon::has_alg(const std::string & name)
{
  if(alg_map)
    return alg_map->count(name) > 0;
  else
    return false;
}

void
StingerMon::get_alg_read_lock()
{
  LOG_D("read lock get");
  pthread_rwlock_rdlock(&alg_lock);
  LOG_D("read lock received");
}

void
StingerMon::release_alg_read_lock()
{
  LOG_D("read lock release");
  pthread_rwlock_unlock(&alg_lock);
  LOG_D("read lock released");
}

void
StingerMon::update_algs(stinger_t * stinger_copy, std::string new_loc, int64_t new_sz, 
  std::vector<StingerAlgState *> * new_algs, std::map<std::string, StingerAlgState *> * new_alg_map,
  const StingerBatch & batch)
{
  LOG_D_A("Called with %s, %ld", new_loc.c_str(), (long)new_sz);
  LOG_D("write lock get");
  pthread_rwlock_wrlock(&alg_lock);
  LOG_D("write lock received");
  /* remap stinger */
  if(stinger) {
    stinger_shared_free(stinger, stinger_loc.c_str(), stinger_sz);
  }
  stinger = stinger_copy;
  stinger_loc = new_loc;
  stinger_sz = new_sz;

  /* unmap / delete existing algs */
  if(algs) {
    for(int64_t i = 0; i < algs->size(); i++) {
      StingerAlgState * cur_alg = (*algs)[i];
      if(cur_alg) {
	shmunmap(cur_alg->data_loc.c_str(), cur_alg->data, cur_alg->data_per_vertex * stinger->max_nv);
	delete cur_alg;
      }
    }

    delete algs;
  }
  if(alg_map)
    delete alg_map;

  algs = new_algs;
  alg_map = new_alg_map;

  /* update max time with latest insertion time */
  int64_t new_max_time = max_time;
  for(int64_t i = 0; i < batch.insertions_size(); i++) {
    int64_t edge_time = batch.insertions(i).time();
    if(edge_time > new_max_time)
      new_max_time = edge_time;
  }
  max_time = new_max_time;

  LOG_D("write lock release");
  pthread_rwlock_unlock(&alg_lock);
  LOG_D("write lock released");
}

int64_t
StingerMon::get_max_time() {
  return max_time;
}

void
StingerMon::wait_for_sync()
{
  readfe((uint64_t *)&wait_lock);
    waiting++;
  writeef((uint64_t *)&wait_lock, 0);

  sem_wait(&sync_lock);
}

void
StingerMon::sync()
{
  readfe((uint64_t *)&wait_lock);
    for(int64_t i = 0; i < waiting; i++)
      sem_post(&sync_lock);
    waiting = 0;
  writeef((uint64_t *)&wait_lock, 0);
}

stinger_t *
StingerMon::get_stinger()
{
  return stinger;
}

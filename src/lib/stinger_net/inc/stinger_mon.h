#ifndef  STINGER_MON_H
#define  STINGER_MON_H

#include <map>
#include <string>
#include <stdint.h>
#include <semaphore.h>
#include "stinger_alg_state.h"
#include "stinger_core/stinger.h"
#include "stinger_core/stinger_error.h"
#include "rapidjson/document.h"

#include "stinger_net/proto/stinger-monitor.pb.h"

namespace gt {
  namespace stinger {
    class StingerMon {

      protected:
	/* I'm a singleton */
	StingerMon();
	~StingerMon();

      private:
	std::vector<StingerAlgState *> * algs;                     
	std::map<std::string, StingerAlgState *> * alg_map;        

	pthread_rwlock_t alg_lock;

	stinger_t * stinger;
	std::string stinger_loc;
	int64_t stinger_sz;

	int64_t waiting;
	int64_t wait_lock;
        sem_t sync_lock;

      public:
	static StingerMon& get_mon();

	size_t
	get_num_algs();

	StingerAlgState *
	get_alg(size_t num);

	StingerAlgState *
	get_alg(const std::string & name);

	bool
	has_alg(const std::string & name);

	void
	get_alg_read_lock();

	void
	release_alg_read_lock();

	void
	update_algs(stinger_t * stinger_copy, std::string new_loc, int64_t new_sz, 
	  std::vector<StingerAlgState *> * new_algs, std::map<std::string, StingerAlgState *> * new_alg_map,
	  const StingerBatch & batch);

	stinger_t *
	get_stinger();

        void
	wait_for_sync();

	void
	sync();
    };
  }
}

#endif  /*STINGER_MON_H*/

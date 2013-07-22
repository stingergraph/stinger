#ifndef  STINGER_SERVER_STATE_H
#define  STINGER_SERVER_STATE_H

#include <pthread.h>
#include <stdint.h>

#include "stinger_stream_state.h"
#include "stinger_alg_state.h"
#include "proto/stinger-batch.pb.h"

/* STL - TODO explore other options */
#include <map>
#include <string>
#include <vector>
#include <queue>

namespace gt {
  namespace stinger {

    /**
    * @brief Singleton class containing global state and configuration
    */
    class StingerServerState {

      private:

	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
	 * PRIVATE PROPERTIES
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	int64_t alg_lock;
	std::vector<StingerAlgState *> algs;                     
	std::map<std::string *, StingerAlgState *> alg_map;        

	int64_t stream_lock;
	std::vector<StingerStreamState *> streams;
	std::map<std::string *, StingerStreamState *> stream_map;

	int64_t batch_lock;
	std::queue<StingerBatch *> batches;

	int port;

	std::vector<pthread_t> threads;
	pthread_t main_loop;

	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
	 * PRIVATE METHODS
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	StingerServerState();

	~StingerServerState();

      public:

	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
	 * PUBLIC METHODS
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	static StingerServerState &
	get_server_state();

	int
	get_port();

	int
	set_port(int new_port);

	pthread_t *
	get_main_loop() { return &main_loop; }

	void
	enqueue_batch(StingerBatch * batch);

	StingerBatch *
	dequeue_batch();

	size_t
	get_num_streams();

	StingerStreamState *
	get_stream(size_t index);

	StingerStreamState *
	get_stream(std::string * name);

	size_t
	get_num_algs();

	StingerAlgState *
	get_alg(size_t index);

	StingerAlgState *
	get_alg(std::string * name);

	pthread_t &
	push_thread(pthread_t & thread);

	void
	set_main_loop_thread(pthread_t thread);
    };

  } /* gt */
} /* stinger */

#endif  /*STINGER_SERVER_STATE_H*/

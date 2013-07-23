#ifndef  STINGER_SERVER_STATE_H
#define  STINGER_SERVER_STATE_H

extern "C" {
  #include "stinger_core/stinger.h"
}

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
	std::vector<std::vector<StingerAlgState *> > alg_tree;
	std::map<std::string, StingerAlgState *> alg_map;        

	int64_t dep_lock;
	std::map<std::string, std::vector<StingerAlgState *> > opt_dependencies;        
	std::map<std::string, std::vector<StingerAlgState *> > req_dependencies;        

	int64_t stream_lock;
	std::vector<StingerStreamState *> streams;
	std::map<std::string, StingerStreamState *> stream_map;

	int64_t batch_lock;
	std::queue<StingerBatch *> batches;

	int port;

	std::vector<pthread_t> threads;
	pthread_t main_loop;

	stinger_t * stinger;

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
	get_stream(std::string & name);

	size_t
	get_num_levels();

	size_t
	get_num_algs();

	size_t
	get_num_algs(size_t level);

	StingerAlgState *
	get_alg(size_t num);

	StingerAlgState *
	get_alg(size_t level, size_t index);

	StingerAlgState *
	get_alg(const std::string & name);


	void
	add_alg(size_t level, StingerAlgState * alg);

	bool
	has_alg(const std::string & name);

	pthread_t &
	push_thread(pthread_t & thread);

	void
	set_main_loop_thread(pthread_t thread);

	void
	set_stinger(stinger_t * S);

	stinger_t *
	get_stinger();
    };

  } /* gt */
} /* stinger */

#endif  /*STINGER_SERVER_STATE_H*/

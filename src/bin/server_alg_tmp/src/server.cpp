extern "C" {
  #include "stinger_core/stinger.h"
  #include "stinger_core/stinger_shared.h"
  #include "stinger_utils/timer.h"
  #include "stinger_utils/stinger_utils.h"
  #include "stinger_core/xmalloc.h"
  #include "stinger_core/x86_full_empty.h"
  #include "stinger_core/stinger_error.h"
}

#include "stinger_net/proto/stinger-batch.pb.h"
#include "stinger_net/proto/stinger-connect.pb.h"
#include "stinger_net/proto/stinger-alg.pb.h"

#include "stinger_net/send_rcv.h"
#include "stinger_net/stinger_server_state.h"

#if !defined(MTA)
#define MTA(x)
#endif

#include <cstdio>
#include <limits>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

/* POSIX only for now, note that mongoose would be a good place to 
get cross-platform threading and sockets code */
#include <pthread.h> 

/* A note on the design of this server:
 * Main thread -> starts all other threads, listens to the incoming port and accepts connections.
 * Stream threads -> One per stream / alg, receives a batch from a stream and enqueues the result. For algorithms, perform the initial setup, then main loop handles
 * Process loop thread -> handles all request
 */

using namespace gt::stinger;

struct AcceptedSock {
  socklen_t len;
  struct sockaddr addr;
  int handle;
};

void
handle_alg(struct AcceptedSock * sock, StingerServerState & server_state)
{
  AlgToServer alg_to_server;
  if(recv_message(sock->handle, alg_to_server)) {

    LOG_D_A("Received new algorithm.  Printing incoming protobuf: %s", alg_to_server.DebugString().c_str());

    ServerToAlg server_to_alg;
    server_to_alg.set_alg_name(alg_to_server.alg_name());
    server_to_alg.set_alg_num(alg_to_server.alg_num());
    server_to_alg.set_action(alg_to_server.action());
    server_to_alg.set_stinger_loc(server_state.get_stinger_loc());
    server_to_alg.set_stinger_size(sizeof(stinger_t) + server_state.get_stinger()->length);

    if(alg_to_server.action() != REGISTER_ALG) {
      LOG_E("Algorithm failed to register");
      /* TODO handle this */
    }

    /* check name and attempt to allocate space */
    if(is_simple_name(alg_to_server.alg_name().c_str(), alg_to_server.alg_name().length())) {
      int64_t data_total = alg_to_server.data_per_vertex() * STINGER_MAX_LVERTICES;
      void * data = NULL;
      char map_name[256] = "";

      if(data_total) {
	sprintf(map_name, "/%s", alg_to_server.alg_name().c_str());
	data = shmmap(map_name, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR, PROT_READ | PROT_WRITE, data_total, MAP_SHARED);

	if(!data) {
	  LOG_E_A("Error, mapping storage for algorithm %s failed (%ld bytes per vertex)", 
	    alg_to_server.alg_name().c_str(), alg_to_server.data_per_vertex());
	  /* TODO handle this */
	  return;
	}
      }

      StingerAlgState * alg_state = new StingerAlgState();
      alg_state->name = alg_to_server.alg_name();
      alg_state->data_loc = map_name;
      alg_state->data = data;
      alg_state->data_per_vertex = alg_to_server.data_per_vertex();
      alg_state->sock_handle = sock->handle;

      int64_t max_level = 0;
      bool deps_resolved = true;
      for(int64_t i = 0; i < alg_to_server.req_dep_name_size(); i++) {
	const std::string & req_dep_name = alg_to_server.req_dep_name(i);
	if(server_state.has_alg(req_dep_name)) {
	  StingerAlgState * dep_state = server_state.get_alg(req_dep_name);
	  int64_t dep_level = dep_state->level;
	  if(dep_level >= max_level) {
	    max_level = dep_level + 1;
	  }
	  server_to_alg.add_dep_name(dep_state->name);
	  server_to_alg.add_dep_description(dep_state->data_description);
	  server_to_alg.add_dep_data_per_vertex(dep_state->data_per_vertex);
	  server_to_alg.add_dep_data_loc(dep_state->data_loc);
	} else {
	  LOG_E_A("Error, dependency %s of alg %s is not available",
	    req_dep_name.c_str(), alg_to_server.alg_name().c_str());
	  deps_resolved = false;
	  /* TODO handle this */
          shmunmap(map_name, data, data_total);
	  delete alg_state;
	  return;
	}
      }

      alg_state->level = max_level;

      server_to_alg.set_alg_num(server_state.add_alg(max_level, alg_state));

      send_message(sock->handle, server_to_alg);
    } else {
      LOG_E_A("Error, requested name is not valid: %s",
	alg_to_server.alg_name().c_str());
      /* TODO handle this */
      return;
    }
  }
}

void *
new_connection_handler(void * data)
{
  StingerServerState & server_state = StingerServerState::get_server_state();
  struct AcceptedSock * accepted_sock = (struct AcceptedSock *)data;

  Connect connect_type;
  recv_message(accepted_sock->handle, connect_type);

  switch(connect_type.type()) {
    case CLIENT_STREAM:
      LOG_W("Client streams are unhandled");
    break;

    case CLIENT_ALG:
      LOG_V("Received client connection");
      handle_alg(accepted_sock, server_state);
    break;

    default:
      LOG_E("Received unknown connection type");
    break;
  }

  delete accepted_sock;
}

void *
process_loop_handler(void * data)
{
  LOG_V("Main loop thread started");

  StingerServerState & server_state = StingerServerState::get_server_state();

  while(1) { /* TODO clean shutdown mechanism */
    StingerBatch * batch = server_state.dequeue_batch();

    /* loop through each algorithm, blocking init as needed, send start preprocessing message */
    size_t stop_alg_level = server_state.get_num_levels();
    for(size_t cur_level_index = 0; cur_level_index < stop_alg_level; cur_level_index++) {
      size_t stop_alg_index = server_state.get_num_algs(cur_level_index);
      for(size_t cur_alg_index = 0; cur_alg_index < stop_alg_index; cur_alg_index++) {
	StingerAlgState * cur_alg = server_state.get_alg(cur_level_index, cur_alg_index);

	if(cur_alg->state == ALG_STATE_READY_INIT) {
	  /* TODO handle initializing the algorithm and the static stuff */
	}

	if(cur_alg->state == ALG_STATE_READY_PRE) {
	  /* TODO handle streaming preprocessing*/
	}
      }

      /* TODO timeout */

      /* loop through each algorithm and wait for message indicating finished preprocesing */
      for(size_t cur_alg_index = 0; cur_alg_index < stop_alg_index; cur_alg_index++) {
	StingerAlgState * cur_alg = server_state.get_alg(cur_level_index, cur_alg_index);

	if(cur_alg->state == ALG_STATE_PERFORMING_PRE) {
	  /* TODO wait on completion message */
	}
      }
    }

    /* TODO update stinger */

    /* loop through each algorithm, blocking init as needed, send start postprocessing message */
    stop_alg_level = server_state.get_num_levels();
    for(size_t cur_level_index = 0; cur_level_index < stop_alg_level; cur_level_index++) {
      size_t stop_alg_index = server_state.get_num_algs(cur_level_index);
      for(size_t cur_alg_index = 0; cur_alg_index < stop_alg_index; cur_alg_index++) {
	StingerAlgState * cur_alg = server_state.get_alg(cur_level_index, cur_alg_index);

	if(cur_alg->state == ALG_STATE_READY_POST) {
	  /* TODO handle streaming postprocessing*/
	}
      }

      /* TODO timeout */

      /* loop through each algorithm and wait for message indicating finished postprocesing */
      for(size_t cur_alg_index = 0; cur_alg_index < stop_alg_index; cur_alg_index++) {
	StingerAlgState * cur_alg = server_state.get_alg(cur_level_index, cur_alg_index);

	if(cur_alg->state == ALG_STATE_PERFORMING_POST) {
	  /* TODO wait for completion of post */
	}
      }
    }
  }
}

int
main(int argc, char *argv[])
{
  LOG_V("Starting up");
  char * stinger_loc = NULL;
  struct stinger * S = stinger_shared_new(&stinger_loc);
  LOG_V("Stinger allocated");

  StingerServerState & server_state = StingerServerState::get_server_state();
  server_state.set_stinger(S);
  server_state.set_stinger_loc(stinger_loc);

  uint64_t buffer_size = 1ULL<<19ULL; /* 512KB - no specific reason for this size */

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:b:"))) {
    switch(opt) {
      case 'p': {
	server_state.set_port(atoi(optarg));
      } break;

      case 'b': {
	buffer_size = atol(optarg);
      } break;

      case '?':
      case 'h': {
	printf("Usage:    %s [-p port] [-b buffer_size]\n", argv[0]);
	printf("Defaults: port: %d buffer_size: %lu\n", server_state.get_port(), (unsigned long) buffer_size);
	exit(0);
      } break;
    }
  }

  LOG_V("Opening the socket");
  int sock_handle;

  struct sockaddr_in sock_addr = {
    .sin_family = AF_INET, 
    .sin_port   = htons((in_port_t)server_state.get_port())
  };

  if(-1 == (sock_handle = socket(AF_INET, SOCK_STREAM, 0))) {
    LOG_F_A("Socket create failed: %s", strerror(errno));
    exit(-1);
  }

  if(-1 == bind(sock_handle, (const sockaddr *)&sock_addr, sizeof(sock_addr))) {
    LOG_F_A("Socket bind failed: %s", strerror(errno));
    exit(-1);
  }

  if(-1 == listen(sock_handle, 16)) {
    LOG_F_A("Socket listen failed: %s", strerror(errno));
    exit(-1);
  }

  LOG_V("Spawning the main loop thread");

  pthread_t main_loop_thread;
  pthread_create(&main_loop_thread, NULL, &process_loop_handler, NULL);
  server_state.set_main_loop_thread(main_loop_thread);

  while(1) {
    struct AcceptedSock * accepted_sock = (struct AcceptedSock *)xcalloc(1,sizeof(struct AcceptedSock));

    LOG_V("Waiting for connections...")
    accepted_sock->handle = accept(sock_handle, &(accepted_sock->addr), &(accepted_sock->len));

    pthread_t new_thread;
    pthread_create(&new_thread, NULL, &new_connection_handler, (void *)accepted_sock);
    server_state.push_thread(new_thread);
  }
}

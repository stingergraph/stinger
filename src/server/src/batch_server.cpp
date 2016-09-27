#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include "server.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_net/stinger_server_state.h"
#include "stinger_core/xmalloc.h"

//#define LOG_AT_D
#include "stinger_core/stinger_error.h"

#define STINGER_MAX_BATCHES 100

typedef struct {
  struct stinger * S;
  int sock;
} handle_stream_args;

void *
handle_stream(void * args)
{
  StingerServerState & server_state = StingerServerState::get_server_state();

  /* should be able to remove these */
  struct stinger *S = ((handle_stream_args *) args)->S;
  int sock = ((handle_stream_args *) args)->sock;
  free(args);

  int nfail = 0;

  LOG_V("Ready to accept messages.");
  while(1)
  {
    StingerBatch * batch = new StingerBatch();
    if (recv_message(sock, *batch)) {
      nfail = 0;

      LOG_V_A("Received message of size %ld", (long)batch->ByteSize());

      if (0 == batch->insertions_size () && 0 == batch->deletions_size ()) {
	LOG_V("Empty batch.");
	if (!batch->keep_alive ()) {
	  delete batch;
	  break;
	} else {
	  delete batch;
	  continue;
	}
      }

      if (server_state.get_queue_size() >= STINGER_MAX_BATCHES) {
	LOG_W("Dropping a batch");
	stinger_uint64_fetch_add(&(S->dropped_batches), 1);
	delete batch;
	continue;
      }
      server_state.enqueue_batch(batch);

      if(!batch->keep_alive()) {
	delete batch;
	break;
      }

    } else {
      ++nfail;
      LOG_E("Parsing failed.");
      if (nfail > 2) {
        delete batch;
        break;
      }
    }
  }
  return NULL;
}

void *
start_batch_server (void * args)
{
  StingerServerState & server_state = StingerServerState::get_server_state();

  struct stinger * S = server_state.get_stinger();
  int port_streams = server_state.get_port_streams();

  int sock_handle, newsockfd;
  pthread_t garbage_thread_handle;

  sock_handle = listen_for_client(port_streams);

  LOG_V_A("STINGER batch server listening for input on port %d", port_streams);

  while (1) {
    newsockfd = accept (sock_handle, NULL, NULL);

    if (newsockfd < 0) {
      perror("Accept new connection failed.\n");
      exit(-1);
    }

    handle_stream_args * args = (handle_stream_args *)xcalloc(1, sizeof(handle_stream_args));
    args->S = S;
    args->sock = newsockfd;

    pthread_create(&garbage_thread_handle, NULL, handle_stream, (void *)args);
  }

  close(sock_handle);
}

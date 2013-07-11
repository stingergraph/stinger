#include <cstdio>
#include <algorithm>
#include <limits>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

extern "C" {
  #include "stinger_core/stinger.h"
  #include "stinger_utils/timer.h"
  #include "stinger_core/xmalloc.h"
}

#include "proto/stinger-batch.pb.h"
#include "server.h"
#include "send_rcv.h"

using namespace gt::stinger;

int
main(int argc, char *argv[])
{
  struct stinger * S = stinger_new();

  /* register edge and vertex types */
  int64_t vtype_twitter_handle;
  stinger_vtype_names_create_type(S, "TwitterHandle", &vtype_twitter_handle);

  int64_t etype_mention;
  stinger_vtype_names_create_type(S, "Mention", &etype_mention);

  /* global options */
  int port = 10101;
  uint64_t buffer_size = 1ULL << 28ULL;

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:b:"))) {
    switch(opt) {
      case 'p': {
	port = atoi(optarg);
      } break;

      case 'b': {
	buffer_size = atol(optarg);
      } break;

      case '?':
      case 'h': {
	printf("Usage:    %s [-p port] [-b buffer_size]\n", argv[0]);
	printf("Defaults: port: %d buffer_size: %lu\n", port, (unsigned long) buffer_size);
	exit(0);
      } break;

    }
  }

  int sock_handle;
  struct sockaddr_in sock_addr = { AF_INET, (in_port_t)port, 0};

  if(-1 == (sock_handle = socket(AF_INET, SOCK_STREAM, 0))) {
    perror("Socket create failed.\n");
    exit(-1);
  }

  if(-1 == bind(sock_handle, (const sockaddr *)&sock_addr, sizeof(sock_addr))) {
    perror("Socket bind failed.\n");
    exit(-1);
  }

  if(-1 == listen(sock_handle, 1)) {
    perror("Socket listen failed.\n");
    exit(-1);
  }

  const char * buffer = (char *)malloc(buffer_size);
  if(!buffer) {
    perror("Buffer alloc failed.\n");
    exit(-1);
  }

  V_A("STINGER server listening for input on port %d, web server on port 8088.",
      (int)port);

  while(1) {
    int accept_handle = accept(sock_handle, NULL, NULL);
    int nfail = 0;

    V("Ready to accept messages.");
    while(1) {

      StingerBatch batch;
      if(recv_message(accept_handle, batch)) {
	nfail = 0;

	V_A("Received message of size %ld", (long)batch.ByteSize());

	if (0 == batch.insertions_size () && 0 == batch.deletions_size ()) {
	  V("Empty batch.");
	  if (!batch.keep_alive ())
	    break;
	  else
	    continue;
	}

	double processing_time_start;
	//batch.PrintDebugString();

//	processing_time_start = timer ();

	//process_batch(S, batch, &cstate);
	process_batch(S, batch, NULL);

//	components_batch(S, STINGER_MAX_LVERTICES, components);

//	cstate_update (&cstate, S);

//	processing_time = timer () - processing_time_start;
//	spdup = (max_batch_ts - min_batch_ts) / processing_time;

	{
	  char mints[100], maxts[100];

	  ts_to_str (min_batch_ts, mints, sizeof (mints)-1);
	  ts_to_str (max_batch_ts, maxts, sizeof (maxts)-1);
	  V_A("Time stamp range: %s ... %s", mints, maxts);
	}

	V_A("Number of non-singleton components %ld/%ld, max size %ld",
	    (long)n_nonsingleton_components, (long)n_components,
	    (long)max_compsize);

/*	V_A("Number of non-singleton communities %ld/%ld, max size %ld, modularity %g",
	    (long)cstate.n_nonsingletons, (long)cstate.cg.nv,
	    (long)cstate.max_csize,
	    cstate.modularity);*/

	V_A("Total time: %ld, processing time: %g, speedup %g",
	    (long)(max_batch_ts-min_batch_ts), processing_time, spdup);

	if(!batch.keep_alive())
	  break;

      } else {
	++nfail;
	V("ERROR Parsing failed.\n");
	if (nfail > 2) break;
      }
    }
    if (nfail > 2) break;
  }

  return 0;
}

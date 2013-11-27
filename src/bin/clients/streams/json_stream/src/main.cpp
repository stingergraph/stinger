#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <time.h>
#include <netdb.h>

#include "stinger_utils/stinger_sockets.h"
#include "stinger_utils/timer.h"
#include "stinger_net/send_rcv.h"
#include "explore_json.h"

using namespace gt::stinger;

int
main(int argc, char *argv[])
{
  /* global options */
  int port = 10102;
  int batch_size = 1000;
  double timeout = 0;
  struct hostent * server = NULL;
  char * filename = NULL;

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:a:x:t:"))) {
    switch(opt) {
      case 'p': {
		  port = atoi(optarg);
		} break;

      case 'x': {
		  batch_size = atol(optarg);
		  LOG_I_A("Batch size changed to %d", batch_size);
		} break;

      case 'a': {
		  server = gethostbyname(optarg);
		  if(NULL == server) {
		    LOG_E_A("ERROR: server %s could not be resolved.", optarg);
		    exit(-1);
		  }
		} break;
      case 't': {
	timeout = atof(optarg);
      } break;

      case '?':
      case 'h': {
		  printf("Usage:    %s [-p port] [-a server_addr] [-t timeout] [-x batch_size] filename\n", argv[0]);
		  printf("Defaults:\n\tport: %d\n\tserver: localhost\n\ttimeout:%lf\n\tbatch_size: %d", port, timeout, batch_size);
		  exit(0);
		} break;
    }
  }

  if (optind < argc && 0 != strcmp (argv[optind], "-")) {
    filename = argv[optind];
  } else {
    LOG_E("No filename given.");
    return -1;
  }

  LOG_V_A("Running with: port: %d\n", port);

  /* connect to localhost if server is unspecified */
  if(NULL == server) {
    server = gethostbyname("localhost");
    if(NULL == server) {
      LOG_E_A("ERROR: server %s could not be resolved.", "localhost");
      exit(-1);
    }
  }

  /* start the connection */
  int sock_handle = connect_to_batch_server (server, port);


  EdgeCollectionSet edge_finder;

  FILE * fp = fopen(filename, "r");
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  rapidjson::Document document;
  while ((linelen = getdelim(&line, &linecap, '\r', fp)) > 0) {
    document.ParseInsitu<0>(line);
    if(document.IsObject())
      edge_finder.learn(document);
  }

  LOG_V("Printing learn");
  edge_finder.print();

  StingerBatch batch;
  batch.set_make_undirected(true);
  batch.set_type(MIXED);
  batch.set_keep_alive(true);

  tic(); 
  double timesince = 0;
  while ((linelen = getdelim(&line, &linecap, '\r', stdin)) > 0) {
    document.ParseInsitu<0>(line);
    if(document.IsObject()) {
      edge_finder.apply(batch, document);
    }
    timesince += toc();
    int64_t total_actions = batch.insertions_size() + batch.deletions_size();
    if(total_actions >= batch_size || (timeout > 0 && timesince >= timeout)) {
      LOG_I_A("Sending a batch of %ld actions", total_actions);
      send_message(sock_handle, batch);
      timesince = 0;
      batch.Clear();
      batch.set_make_undirected(true);
      batch.set_type(MIXED);
      batch.set_keep_alive(true);
      sleep(2);
    }
  }

  int64_t total_actions = batch.insertions_size() + batch.deletions_size();
  if(total_actions) {
    LOG_I_A("Sending a batch of %ld actions", total_actions);
    send_message(sock_handle, batch);
  }

  return 0;
}

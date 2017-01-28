#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <sys/types.h>
#include <time.h>
#include <netdb.h>

#include "stinger_utils/timer.h"
#include "stinger_net/send_rcv.h"
#include "stinger_utils/csv.h"
#include "explore_csv.h"

using namespace gt::stinger;

#define E_A(X,...) fprintf(stderr, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define E(X) E_A(X,NULL)
#define V_A(X,...) fprintf(stdout, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define V(X) V_A(X,NULL)

#define LOG_AT_I 1
#include "stinger_core/stinger_error.h"
#include "stinger_core/formatting.h"


int
main(int argc, char *argv[])
{
  /* global options */
  int port = 10102;
  int batch_size = 1000;
  double timeout = 0;
  char const * hostname = NULL;
  char * filename = NULL;
  int use_directed = 0;

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:a:x:t:d"))) {
    switch(opt) {
      case 'p': {
		  port = atoi(optarg);
		} break;

      case 'x': {
		  batch_size = atol(optarg);
		  LOG_I_A("Batch size changed to %d", batch_size);
		} break;

      case 'a': {
		  hostname = optarg;
		} break;

      case 'd': {
		  use_directed = 1;
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
  if(NULL == hostname) {
    hostname = "localhost";
  }

  /* start the connection */
  int sock_handle = connect_to_server (hostname, port);

  if (sock_handle == -1) exit(-1);


  EdgeCollectionSet edge_finder;

  FILE * fp = fopen(filename, "r");
  char * buf = NULL, ** fields = NULL;
  uint64_t bufSize = 0, * lengths = NULL, fieldsSize = 0, count = 0;

  while (!feof(fp)) {
    readCSVLineDynamic(',', fp, &buf, &bufSize, &fields, &lengths, &fieldsSize, &count);
    if (count <= 1)
      continue;
    edge_finder.learn(fields, (int64_t *)lengths, count);
  }

  LOG_V("Printing learn");
  edge_finder.print();

  StingerBatch batch;
  if(use_directed) {
    batch.set_make_undirected(false);
  } else {
    batch.set_make_undirected(true);
  }
  batch.set_type(MIXED);
  batch.set_keep_alive(true);

  tic(); 
  double timesince = 0;
  while (!feof(stdin)) {
    int64_t count_read = readCSVLineDynamic(',', stdin, &buf, &bufSize, &fields, &lengths, &fieldsSize, &count);
    if (count > 1) {
      if(edge_finder.apply(batch, fields, (int64_t *)lengths, count, batch.metadata_size())) {
	batch.add_metadata(buf, count_read);
      }
    }
    timesince += toc();
    int64_t total_actions = batch.insertions_size() + batch.deletions_size();
    if(total_actions >= batch_size || (timeout > 0 && timesince >= timeout)) {
      LOG_I_A("Sending a batch of %" PRId64 " actions", total_actions);
      send_message(sock_handle, batch);
      timesince = 0;
      batch.Clear();
      if(use_directed) {
	batch.set_make_undirected(false);
      } else {
	batch.set_make_undirected(true);
      }
      batch.set_type(MIXED);
      batch.set_keep_alive(true);
    }
  }

  int64_t total_actions = batch.insertions_size() + batch.deletions_size();
  if(total_actions) {
    LOG_I_A("Sending a batch of %" PRId64 " actions", total_actions);
    send_message(sock_handle, batch);
  }

  free(buf); free(fields); free(lengths);
  return 0;
}

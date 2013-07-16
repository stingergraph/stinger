extern "C" {
#include "stinger_core/stinger.h"
#include "stinger_utils/csv.h"
#include "stinger_utils/timer.h"
}

#include "proto/random_edge_generator.pb.h"
#include "random_edge_generator.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <netdb.h>

using namespace gt::stinger;

#define E_A(X,...) fprintf(stderr, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define E(X) E_A(X,NULL)
#define V_A(X,...) fprintf(stdout, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define V(X) V_A(X,NULL)


int
main(int argc, char *argv[])
{
  /* global options */
  int port = 10101;
  int batch_size = 100000;
  int num_batches = -1;
  uint64_t buffer_size = 1ULL << 28ULL;
  struct hostent * server = NULL;

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:b:a:x:y:"))) {
    switch(opt) {
      case 'p': {
	port = atoi(optarg);
      } break;

      case 'b': {
	buffer_size = atol(optarg);
      } break;

      case 'x': {
	batch_size = atol(optarg);
      } break;

      case 'y': {
	num_batches = atol(optarg);
      } break;

      case 'a': {
	server = gethostbyname(optarg);
	if(NULL == server) {
	  E_A("ERROR: server %s could not be resolved.", optarg);
	  exit(-1);
	}
      } break;

      case '?':
      case 'h': {
	printf("Usage:    %s [-p src_port] [-d dst_port] [-a server_addr] [-b buffer_size] [-s for strings] [-x batch_size] [-y num_batches] [-i in_seconds] [-o out_seconds] [-j] [<input file>]\n", argv[0]);
	printf("Defaults: port: %d server: localhost buffer_size: %lu\n", port, (unsigned long) buffer_size);
	printf("\nCSV file format is is_delete,source,dest,weight,time where weight and time are\n"
		 "optional 64-bit integers and source and dest are either strings or\n"
		 "64-bit integers depending on options flags. is_delete should be 0 for edge\n"
		 "insertions and 1 for edge deletions.\n"
		 "\n"
		 "-j turns on JSON parsing mode, which expects a JSON twitter file rather than CSV\n"
		 "   this file will be turned into a graph of mentions\n"
		 "-i controls how many seconds of the input file will be read into one batch\n"
		 "-o controls how often batches will be sent in seconds\n"
		 "   using both -i and -o allows you to control the playback rate of the file (with -i 1 -o 1\n"
		 "   being read one second of Twitter data and send it once per second)\n");
	exit(0);
      } break;
    }
  }

  V_A("Running with: port: %d buffer_size: %lu\n", port, (unsigned long) buffer_size);

  if(NULL == server) {
    server = gethostbyname("localhost");
    if(NULL == server) {
      E_A("ERROR: server %s could not be resolved.", "localhost");
      exit(-1);
    }
  }

  int sock_handle, n;
  struct sockaddr_in serv_addr;

  if (-1 == (sock_handle = socket(AF_INET, SOCK_STREAM, 0))) {
    perror("Socket create failed");
    exit(-1);
  }

  bzero ((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy ((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(port);

  uint8_t * buffer = (uint8_t *) malloc (buffer_size);
  if(!buffer) {
    perror("Buffer alloc failed");
    exit(-1);
  }

  if(-1 == connect(sock_handle, (const sockaddr *) &serv_addr, sizeof(serv_addr))) {
    perror("Connection failed");
    exit(-1);
  }


  char * buf = NULL, ** fields = NULL;
  uint64_t bufSize = 0, * lengths = NULL, fieldsSize = 0, count = 0;
  int64_t line = 0;
  int batch_num = 0;

  while(1) {
    V("Parsing messages.");

    StingerBatch batch;
    batch.set_make_undirected(true);
    batch.set_type(NUMBERS_ONLY);
    batch.set_keep_alive(true);

    for(int e = 0; e < batch_size; e++) {
      line++;

      /* is insert? */
      EdgeInsertion * insertion = batch.add_insertions();
      insertion->set_source(1);
      insertion->set_destination(10);
      insertion->set_weight(1);
      insertion->set_time(line);
    }

    V("Sending messages.");

    send_message(sock_handle, batch);
    sleep(2);

    if((batch_num >= num_batches) && (num_batches != -1)) {
      break;
    } else {
      batch_num++;
    }
    batch_num++;
  }

  StingerBatch batch;
  batch.set_make_undirected(true);
  batch.set_type(NUMBERS_ONLY);
  batch.set_keep_alive(false);
  send_message(sock_handle, batch);

  free(buffer);
  return 0;
}

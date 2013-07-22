#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <time.h>
#include <netdb.h>

extern "C" {
#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_utils/csv.h"
#include "stinger_utils/timer.h"
#include "stinger_utils/stinger_sockets.h"
}

#include "csv_stream.h"

using namespace gt::stinger;

#define E_A(X,...) fprintf(stderr, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define E(X) E_A(X,NULL)
#define V_A(X,...) fprintf(stdout, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define V(X) V_A(X,NULL)

enum csv_fields {
  FIELD_IS_DELETE,
  FIELD_SOURCE,
  FIELD_DEST,
  FIELD_WEIGHT,
  FIELD_TIME,
  FIELD_TYPE
};

  int
main(int argc, char *argv[])
{
  /* global options */
  int port = 10101;
  int batch_size = 100000;
  int num_batches = -1;
  uint64_t buffer_size = 1ULL << 28ULL;
  struct hostent * server = NULL;
  char * filename = NULL;
  int use_numerics = 0;

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:b:a:x:y:1"))) {
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

      case '1': {
		  use_numerics = 1;
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
		  printf("Usage:    %s [-p port] [-a server_addr] [-b buffer_size] [-x batch_size] [-y num_batches]\n", argv[0]);
		  printf("Defaults:\n\tport: %d\n\tserver: localhost\n\tbuffer_size: %lu\n", port, (unsigned long) buffer_size);
		  exit(0);
		} break;
    }
  }

  if (optind < argc && 0 != strcmp (argv[optind], "-"))
    filename = argv[optind];

  V_A("Running with: port: %d buffer_size: %lu\n", port, (unsigned long) buffer_size);

  /* connect to localhost if server is unspecified */
  if(NULL == server) {
    server = gethostbyname("localhost");
    if(NULL == server) {
      E_A("ERROR: server %s could not be resolved.", "localhost");
      exit(-1);
    }
  }

  /* start the connection */
  int sock_handle = connect_to_batch_server (server, port);

  uint8_t * buffer = (uint8_t *) xmalloc (buffer_size);
  if(!buffer) {
    perror("Buffer alloc failed");
    exit(-1);
  }

  /* actually generate and send the batches */
  char * buf = NULL, ** fields = NULL;
  uint64_t bufSize = 0, * lengths = NULL, fieldsSize = 0, count = 0;
  int64_t line = 0;
  int batch_num = 0;

  FILE * fp = (filename ? fopen(filename, "r") : stdin);
  if (!fp) {
    char errmsg[257];
    snprintf (errmsg, 256, "Opening \"%s\" failed", filename);
    errmsg[256] = 0;
    perror (errmsg);
    exit (-1);
  }

  /* read the lines */
  StingerBatch batch;
  batch.set_make_undirected(true);
  batch.set_type(use_numerics ? NUMBERS_ONLY : STRINGS_ONLY);
  batch.set_keep_alive(true);

  while (!feof(fp)) {
    line++;
    readCSVLineDynamic(',', fp, &buf, &bufSize, &fields, &lengths, &fieldsSize, &count);

    if (count <= 1)
      continue;
    if (count < 3) {
      E_A("ERROR: too few elements on line %ld", (long) line);
      continue;
    }

    /* is insert? */
    if (fields[FIELD_IS_DELETE][0] == '0') {
      EdgeInsertion * insertion = batch.add_insertions();

      if (!use_numerics) {
	insertion->set_source_str(fields[FIELD_SOURCE]);
	insertion->set_destination_str(fields[FIELD_DEST]);
      } else {
	insertion->set_source(atol(fields[FIELD_SOURCE]));
	insertion->set_destination(atol(fields[FIELD_DEST]));
      }

      if (count > 3) {
	insertion->set_weight(atol(fields[FIELD_WEIGHT]));
	if (count > 4) {
	  insertion->set_time(atol(fields[FIELD_TIME]));
	  if (count > 5) {
	    insertion->set_type_str(fields[FIELD_TYPE]);
	  }
	}
      }

    } else {
      EdgeDeletion * deletion = batch.add_deletions();

      if (!use_numerics) {
	deletion->set_source_str(fields[FIELD_SOURCE]);
	deletion->set_destination_str(fields[FIELD_DEST]);
      } else {
	deletion->set_source(atol(fields[FIELD_SOURCE]));
	deletion->set_destination(atol(fields[FIELD_DEST]));
      }
      if (count > 5) {
	deletion->set_type_str(fields[FIELD_TYPE]);
      }
    }

    if (!(line % batch_size))
    {
      V("Sending messages.");
      send_message(sock_handle, batch);
    }
  }


  sleep(2);



  batch.set_make_undirected(true);
  batch.set_type(NUMBERS_ONLY);
  batch.set_keep_alive(false);
  send_message(sock_handle, batch);

  free(buffer);
  return 0;
}

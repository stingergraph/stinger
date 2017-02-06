#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <time.h>
#include <netdb.h>

#ifdef __cplusplus
#if !defined(INT64_MAX)
# include <limits>
# define INT64_MAX std::numeric_limits<int64_t>::max()
#endif // INT64_MAX
#endif // __cplusplus

extern "C" {
#include "stinger_core/stinger.h"
#include "stinger_core/formatting.h"
#include "stinger_core/xmalloc.h"
#include "stinger_utils/csv.h"
#include "stinger_utils/timer.h"
}

#include "random_edge_generator.h"
#include "build_name.h"


using namespace gt::stinger;

#define E_A(X,...) fprintf(stderr, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define E(X) E_A(X,NULL)
#define V_A(X,...) fprintf(stdout, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
#define V(X) fprintf(stdout, "%s %s %d:\n\t" #X "\n", __FILE__, __func__, __LINE__);

#define DEFAULT_SEED 0x9367

int
main(int argc, char *argv[])
{
  /* global options */
  int port = 10102;
  long batch_size = 100000;
  long num_batches = -1;
  int64_t nv = 1024;
  int is_int = 0;
  int delay = 2;
  long seed = DEFAULT_SEED;
  char const * hostname = NULL;

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:b:a:x:y:n:is:d:"))) {
    switch(opt) {
      case 'p': {
	port = atoi(optarg);
      } break;

      case 'x': {
	batch_size = atol(optarg);
      } break;

      case 'y': {
	num_batches = atol(optarg);
      } break;

      case 'd': {
	delay = atoi (optarg);
      } break;

      case 's': {
        long tmp_seed = atol (optarg);
        if (tmp_seed) seed = tmp_seed;
        else seed = time (NULL);
      } break;

      case 'i': {
	is_int = 1;
      } break;

      case 'a': {
	hostname = optarg;
      } break;

      case 'n': {
	nv = atol(optarg);
      } break;

      case '?':
      case 'h': {
	printf("Usage:    %s [-p port] [-a server_addr] [-n num_vertices] [-x batch_size] [-y num_batches] [-i] [-s seed] [-d delay]\n", argv[0]);
	printf("Defaults:\n\tport: %d\n\tserver: localhost\n\tnum_vertices: %" PRIu64 "\n -i forces the use of integers in place of strings\n", port, nv);
	exit(0);
      } break;
    }
  }

  if (nv > 1UL<<31) {
    fprintf (stderr, "generator does not support nv > 2**31  (requested %ld)\n", (long)nv);
    return EXIT_FAILURE;
  }

  V_A("Running with: port: %d\n", port);

  /* connect to localhost if server is unspecified */
  if(NULL == hostname) {
    hostname = "localhost";
  }

  /* start the connection */
  int sock_handle = connect_to_server (hostname, port);
  if (sock_handle == -1) exit(-1);

  /* actually generate and send the batches */
  /*char * buf = NULL, ** fields = NULL;
  uint64_t bufSize = 0, * lengths = NULL, fieldsSize = 0, count = 0;*/
  int64_t line = 0;
  int batch_num = 0;

  srand48 (seed);
  const lldiv_t nv_breakup = lldiv (INT64_MAX, nv);

  while(1) {
    StingerBatch batch;
    batch.set_make_undirected(true);
    batch.set_type(is_int ? NUMBERS_ONLY : STRINGS_ONLY);
    batch.set_keep_alive(true);

    std::string src, dest;

    // IF YOU MAKE THIS LOOP PARALLEL, 
    // MOVE THE SRC/DEST STRINGS ABOVE
    for(int e = 0; e < batch_size; e++) {
      line++;

      int64_t u, v;
      /* Rejection samping for u and v */
      while (1) {
        int64_t tmp = lrand48 ();
        if (tmp >= nv_breakup.rem) {
          u = tmp % nv;
          break;
        }
      }
      while (1) {
        int64_t tmp = lrand48 ();
        if (tmp >= nv_breakup.rem) {
          v = tmp % nv;
          break;
        }
      }

      if(u == v) {
	e--;
	line--;
	continue;
      }

      /* is insert? */
      EdgeInsertion * insertion = batch.add_insertions();
      if(is_int) {
	insertion->set_source(u);
	insertion->set_destination(v);
      } else {
	/* Jason Riedy commented these out in d9c4ac2. */
	/* This switches between generating person-like names for vertices
	 * and generating strings from vertex IDs. */
	// insertion->set_source_str(build_name(src, u));
	// insertion->set_destination_str(build_name(dest, v));
        std::ostringstream foo;
        foo << u;
        src = foo.str();
        foo.str("");
        foo << v;
        dest = foo.str();
        // std::cerr << "Edge  " << src << " -- " << dest << std::endl;
	insertion->set_source_str(src);
	insertion->set_destination_str(dest);
      }
      insertion->set_weight(1);
      insertion->set_time(line);
    }

    V("Sending messages.");

    send_message(sock_handle, batch);
    sleep(delay);

    batch_num++;
    if((batch_num >= num_batches) && (num_batches != -1)) {
      break;
    }
  }

  StingerBatch batch;
  batch.set_make_undirected(true);
  batch.set_type(is_int ? NUMBERS_ONLY : STRINGS_ONLY);
  batch.set_keep_alive(false);
  send_message(sock_handle, batch);

  return 0;
}

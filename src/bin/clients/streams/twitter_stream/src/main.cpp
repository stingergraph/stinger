#include <cstdlib>
#include <cstring>
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

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "twitter_stream.h"

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
  struct hostent * server = NULL;
  char * filename = NULL;

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:a:x"))) {
    switch(opt) {
      case 'p': {
		  port = atoi(optarg);
		} break;

      case 'x': {
		  batch_size = atol(optarg);
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
		  printf("Usage:    %s [-p port] [-a server_addr] [-x batch_size] filename\n", argv[0]);
		  printf("Defaults:\n\tport: %d\n\tserver: localhost\n\tbatch_size: %d", port, batch_size);
		  exit(0);
		} break;
    }
  }

  if (optind < argc && 0 != strcmp (argv[optind], "-"))
    filename = argv[optind];

  V_A("Running with: port: %d\n", port);

  /* connect to localhost if server is unspecified */
  if(NULL == server) {
    server = gethostbyname("localhost");
    if(NULL == server) {
      E_A("ERROR: server %s could not be resolved.", "localhost");
      exit(-1);
    }
  }

  /* start the connection */
//  int sock_handle = connect_to_batch_server (server, port);


  /* load the template file */
  std::map<int, std::list<std::string> > found;

  load_template_file (filename, '\r', found);

  printf("Here are the results:\n\n");
  for (int64_t i = 0; i < 5; i++) {
    printf("%ld: ", i);
    print_list (found[i]);
  }

  assert(found.size() == 5);


  /* begin parsing stdin */
  std::list<std::string> breadcrumbs;
  std::string line;

  /* read the lines */
  while (std::getline (std::cin, line, '\r'))
  {
    if (!std::cin)
      break;

    rapidjson::Document document;
    document.Parse<0>(line.c_str());

    test_describe_object (document, breadcrumbs, found, 0);

  }








  /*
     StingerBatch batch;
     batch.set_make_undirected(true);
     batch.set_type(NUMBERS_ONLY);
     batch.set_keep_alive(false);
     send_message(sock_handle, batch);
   */
  return 0;
}

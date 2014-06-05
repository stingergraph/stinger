#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <time.h>
#include <netdb.h>
#include <tclap/CmdLine.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_utils/csv.h"
#include "stinger_utils/timer.h"
#include "stinger_utils/stinger_sockets.h"
#include "stinger_core/stinger_error.h"

#include "random_edge_generator.h"
#include "build_name.h"

using namespace gt::stinger;


int
main(int argc, char *argv[])
{
  /* global options */
  int port;
  int batch_size;
  int num_batches;
  int nv;
  bool is_int;
  struct hostent * server = NULL;

  try {
    /* parse command line configuration */
    TCLAP::CmdLine cmd("STINGER Uniform Random Edge Generation Stream", ' ', "1.0");
    
    TCLAP::ValueArg<std::string> hostnameArg ("a", "host", "STINGER Server hostname", false, "localhost", "hostname", cmd);
    TCLAP::ValueArg<int> portArg ("p", "port", "STINGER Stream Port", false, 10102, "port", cmd);
    TCLAP::ValueArg<int> batchArg ("x", "batchsize", "Number of edges per batch", false, 1000, "edges", cmd);
    TCLAP::ValueArg<int> numBatchesArg ("y", "numbatches", "Number of batches to send", false, -1, "batches", cmd);
    TCLAP::ValueArg<int> nvArg ("n", "numvertices", "Number of possible vertices", false, 1024, "vertices", cmd);
    TCLAP::SwitchArg intSwitch ("i", "integers", "Set to generate integer vertices only (no names)", cmd, false);

    cmd.parse (argc, argv);

    port = portArg.getValue();
    batch_size = batchArg.getValue();
    num_batches = numBatchesArg.getValue();
    nv = nvArg.getValue();
    is_int = intSwitch.getValue();
    
    server = gethostbyname(hostnameArg.getValue().c_str());
    if(NULL == server) {
      LOG_E_A("Hostname %s could not be resolved.", hostnameArg.getValue().c_str());
      exit(-1);
    }
    
    if(!(nv > 0)) {
      LOG_E("ERROR: incorrect number of vertices");
      exit(-1);
    }
    
  } catch (TCLAP::ArgException &e)
  { std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; return 0; }


  /* start the connection */
  int sock_handle = connect_to_batch_server (server, port);
  LOG_V_A("Connected to %s on port %d", server->h_name, port);

  /* actually generate and send the batches */
  char * buf = NULL, ** fields = NULL;
  uint64_t bufSize = 0, * lengths = NULL, fieldsSize = 0, count = 0;
  int64_t line = 0;
  int batch_num = 0;

  srand (time(NULL));

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

      int64_t u = rand() % nv;
      int64_t v = rand() % nv;

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
	insertion->set_source_str(build_name(src, u));
	insertion->set_destination_str(build_name(dest, v));
      }
      insertion->set_weight(1);
      insertion->set_time(line);
    }

    LOG_V("Sending messages.");

    send_message(sock_handle, batch);
    sleep(2);

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

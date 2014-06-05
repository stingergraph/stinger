#include <iostream>
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
#include "stinger_utils/stinger_sockets.h"
#include "stinger_utils/timer.h"

#include "human_edge_generator.h"

using namespace gt::stinger;


int
main(int argc, char *argv[])
{
  /* global options */
  int port;
  struct hostent * server = NULL;

  try {
    /* parse command line configuration */
    TCLAP::CmdLine cmd("STINGER Human-generated Edge Stream", ' ', "1.0");
    
    TCLAP::ValueArg<std::string> hostnameArg ("a", "host", "STINGER Server hostname", false, "localhost", "hostname", cmd);
    TCLAP::ValueArg<int> portArg ("p", "port", "STINGER Stream Port", false, 10102, "port", cmd);
    
    cmd.parse (argc, argv);

    port = portArg.getValue();
    
    server = gethostbyname(hostnameArg.getValue().c_str());
    if(NULL == server) {
      LOG_E_A("Hostname %s could not be resolved.", hostnameArg.getValue().c_str());
      exit(-1);
    }
    
  } catch (TCLAP::ArgException &e)
  { std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; return 0; }
  

  /* start the connection */
  int sock_handle = connect_to_batch_server (server, port);
  LOG_V_A("Connected to %s on port %d", server->h_name, port);

  /* actually generate and send the batches */
  int64_t batch_num = 0;
  int64_t time = 0;

  while (1)
  {
    StingerBatch batch;
    batch.set_make_undirected(true);
    batch.set_type(STRINGS_ONLY);
    batch.set_keep_alive(true);

    std::string src, dest;
    std::cout << "\nEnter the source vertex: ";
    std::getline (std::cin, src);

    std::cout << "Enter the destination vertex: ";
    std::getline (std::cin, dest);

    time = get_current_timestamp();

    EdgeInsertion * insertion = batch.add_insertions();
    insertion->set_source_str(src);
    insertion->set_destination_str(dest);
    insertion->set_weight(1);
    insertion->set_time(time);

    LOG_I_A ("%ld: <%s, %s> at time %ld", batch_num, src.c_str(), dest.c_str(), time);
    LOG_D ("Sending message");
    send_message(sock_handle, batch);

    batch_num++;
    
  }

  StingerBatch batch;
  batch.set_make_undirected(true);
  batch.set_type(STRINGS_ONLY);
  batch.set_keep_alive(false);
  send_message(sock_handle, batch);

  return 0;
}

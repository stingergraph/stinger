#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <time.h>
#include <netdb.h>
#include <tclap/CmdLine.h>

#include "stinger_utils/stinger_sockets.h"
#include "stinger_utils/timer.h"
#include "stinger_net/send_rcv.h"
#include "explore_json.h"

#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


int
main(int argc, char *argv[])
{
  /* global options */
  int port;
  int batch_size;
  struct hostent * server = NULL;
  bool use_directed;
  float timeout;
  const char * filename = NULL;

  try {
    /* parse command line configuration */
    TCLAP::CmdLine cmd("STINGER Templated JSON Stream", ' ', "1.0");
    
    TCLAP::ValueArg<std::string> hostnameArg ("a", "host", "STINGER Server hostname", false, "localhost", "hostname");
    TCLAP::ValueArg<int> portArg ("p", "port", "STINGER Stream Port", false, 10102, "port");
    TCLAP::ValueArg<int> batchArg ("x", "batchsize", "Number of edges per batch", false, 1000, "edges");
    TCLAP::ValueArg<float> timeoutArg ("t", "timeout", "Timeout", false, 0.0, "seconds");
    TCLAP::SwitchArg directedSwitch ("d", "directed", "Set if graph edges are directed.  Otherwise edges are assumed undirected.", cmd, false);
    TCLAP::UnlabeledValueArg<std::string> filenameArg ("template", "Path to CSV template file", true, "", "filename");

    cmd.parse (argc, argv);

    port = portArg.getValue();
    batch_size = batchArg.getValue();
    use_directed = directedSwitch.getValue();
    timeout = timeoutArg.getValue();
    filename = filenameArg.getValue().c_str();
    
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
  if(use_directed) {
    batch.set_make_undirected(false);
  } else {
    batch.set_make_undirected(true);
  }
  batch.set_type(MIXED);
  batch.set_keep_alive(true);

  tic(); 
  double timesince = 0;
  while ((linelen = getdelim(&line, &linecap, '\r', stdin)) > 0) {
    document.Parse<0>(line);
    if(document.IsObject()) {
      if(edge_finder.apply(batch, document, batch.metadata_size())) {
	batch.add_metadata(line);
      }
    }
    timesince += toc();
    int64_t total_actions = batch.insertions_size() + batch.deletions_size();
    if(total_actions >= batch_size || (timeout > 0 && timesince >= timeout)) {
      LOG_I_A("Sending a batch of %ld actions", total_actions);
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
      sleep(5);
    }
  }

  int64_t total_actions = batch.insertions_size() + batch.deletions_size();
  if(total_actions) {
    LOG_I_A("Sending a batch of %ld actions", total_actions);
    send_message(sock_handle, batch);
  }

  return 0;
}

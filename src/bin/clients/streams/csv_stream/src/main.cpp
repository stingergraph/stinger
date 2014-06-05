#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <time.h>
#include <netdb.h>
#include <tclap/CmdLine.h>

#include "stinger_utils/stinger_sockets.h"
#include "stinger_utils/timer.h"
#include "stinger_net/send_rcv.h"
#include "stinger_utils/csv.h"
#include "explore_csv.h"

#define LOG_AT_I 1
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
    TCLAP::CmdLine cmd("STINGER Templated CSV Stream", ' ', "1.0");
    
    TCLAP::ValueArg<std::string> hostnameArg ("a", "host", "STINGER Server hostname", false, "localhost", "hostname", cmd);
    TCLAP::ValueArg<int> portArg ("p", "port", "STINGER Stream Port", false, 10102, "port", cmd);
    TCLAP::ValueArg<int> batchArg ("x", "batchsize", "Number of edges per batch", false, 1000, "edges", cmd);
    TCLAP::ValueArg<float> timeoutArg ("t", "timeout", "Timeout", false, 0.0, "seconds", cmd);
    TCLAP::SwitchArg directedSwitch ("d", "directed", "Set if graph edges are directed.  Otherwise edges are assumed undirected.", cmd, false);
    TCLAP::UnlabeledValueArg<std::string> filenameArg ("template", "Path to CSV template file", true, "", "filename", cmd);

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
  if (fp == NULL) {
    LOG_E_A ("Unable to open file: %s", filename);
    exit(-1);
  }

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
    }
  }

  int64_t total_actions = batch.insertions_size() + batch.deletions_size();
  if(total_actions) {
    LOG_I_A("Sending a batch of %ld actions", total_actions);
    send_message(sock_handle, batch);
  }

  free(buf); free(fields); free(lengths);
  return 0;
}

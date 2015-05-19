#include <time.h>
#include <netdb.h>
#include <string>
#include <sys/types.h>

#include "proto/stinger-batch.pb.h"
#include "stinger_utils/stinger_sockets.h"
#include "stinger_alg.h"
#include "send_rcv.h"

using namespace gt::stinger;

extern "C" int stream_connect(char * host, int port) {
  struct hostent * server = NULL;
  if(host && strlen(host)) {
    server = gethostbyname(host);
  } else {
    server = gethostbyname("localhost");
  }
  return connect_to_batch_server (server, port);
}

extern "C" void stream_send_batch(int sock_handle, int only_strings,
    stinger_edge_update * insertions, int64_t num_insertions,
    stinger_edge_update * deletions, int64_t num_deletions, bool directed) {

  StingerBatch batch;
  batch.set_make_undirected(directed);
  batch.set_keep_alive(true);
  batch.set_type(only_strings ? STRINGS_ONLY : NUMBERS_ONLY);

  if(insertions) {
    if(only_strings) {
      for(int e = 0; e < num_insertions; e++) {
	EdgeInsertion * insertion = batch.add_insertions();
	if(insertions[e].type_str) {
	  insertion->set_type_str(insertions[e].type_str);
	} else {
	  insertion->set_type(insertions[e].type);
	}
	insertion->set_source_str(insertions[e].source_str);
	insertion->set_destination_str(insertions[e].destination_str);
	insertion->set_weight(insertions[e].weight);
	insertion->set_time(insertions[e].time);
      }
    } else {
      for(int e = 0; e < num_insertions; e++) {
	EdgeInsertion * insertion = batch.add_insertions();
	if(insertions[e].type_str) {
	  insertion->set_type_str(insertions[e].type_str);
	} else {
	  insertion->set_type(insertions[e].type);
	}
	insertion->set_source(insertions[e].source);
	insertion->set_destination(insertions[e].destination);
	insertion->set_weight(insertions[e].weight);
	insertion->set_time(insertions[e].time);
      }
    }
  }

  if(deletions) {
    if(only_strings) {
      for(int e = 0; e < num_deletions; e++) {
	EdgeDeletion * deletion = batch.add_deletions();
	if(deletions[e].type_str) {
	  deletion->set_type_str(deletions[e].type_str);
	} else {
	  deletion->set_type(deletions[e].type);
	}
	deletion->set_source_str(deletions[e].source_str);
	deletion->set_destination_str(deletions[e].destination_str);
      }
    } else {
      for(int e = 0; e < num_deletions; e++) {
	EdgeDeletion * deletion = batch.add_deletions();
	if(deletions[e].type_str) {
	  deletion->set_type_str(deletions[e].type_str);
	} else {
	  deletion->set_type(deletions[e].type);
	}
	deletion->set_source(deletions[e].source);
	deletion->set_destination(deletions[e].destination);
      }
    }
  }

  send_message(sock_handle, batch);
}

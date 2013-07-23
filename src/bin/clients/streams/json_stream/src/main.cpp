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

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"
#include "rapidjson/prettywriter.h"
#include "json_stream.h"

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
  int use_numerics = 0;

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:a:x:1"))) {
    switch(opt) {
      case 'p': {
		  port = atoi(optarg);
		} break;

      case 'x': {
		  batch_size = atol(optarg);
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
  int sock_handle = connect_to_batch_server (server, port);

  /* actually generate and send the batches */
  char * line = NULL;
  size_t line_len = 0;
  int64_t line_number = 0;

  FILE * fp = (filename ? fopen(filename, "r") : stdin);
  if (!fp) {
    char errmsg[257];
    snprintf (errmsg, 256, "Opening \"%s\" failed", filename);
    errmsg[256] = 0;
    perror (errmsg);
    exit (-1);
  }

  /* read the lines */
  while (!feof(fp))
  {
    rapidjson::Document document;
    StingerBatch batch;
    batch.set_make_undirected(true);
    batch.set_type(use_numerics ? NUMBERS_ONLY : STRINGS_ONLY);
    batch.set_keep_alive(true);

    line_number++;
    getline(&line, &line_len, fp);
    document.Parse<0>(line);

    assert(document.IsObject());

    assert(document.HasMember("is_delete"));
    assert(document["is_delete"].IsNumber());
    assert(document["is_delete"].IsInt());
    int64_t is_delete = document["is_delete"].GetInt();

    if (!is_delete)
    {
      /* insertion */
      EdgeInsertion * insertion = batch.add_insertions();

      assert(document.HasMember("source"));
      assert(document["source"].IsString());
      assert(document.HasMember("target"));
      assert(document["target"].IsString());
      insertion->set_source_str(document["source"].GetString());
      insertion->set_destination_str(document["target"].GetString());

      if (document.HasMember("weight")) {
	if (document["weight"].IsNumber() && document["weight"].IsInt()) {
	  insertion->set_weight(document["weight"].GetInt());
	}
      }

      if (document.HasMember("time")) {
	if (document["time"].IsNumber() && document["time"].IsInt()) {
	  insertion->set_time(document["time"].GetInt());
	}
      }

      if (document.HasMember("type")) {
	if (document["type"].IsString()) {
	  insertion->set_type_str(document["type"].GetString());
	}
      }

    }
    else
    {
      /* deletion */
      EdgeDeletion * deletion = batch.add_deletions();

      assert(document.HasMember("source"));
      assert(document["source"].IsString());
      assert(document.HasMember("target"));
      assert(document["target"].IsString());
      deletion->set_source_str(document["source"].GetString());
      deletion->set_destination_str(document["target"].GetString());

      if (document.HasMember("type")) {
	if (document["type"].IsString()) {
	  deletion->set_type_str(document["type"].GetString());
	}
      }

    }

    if (!(line_number % batch_size))
    {
      V("Sending messages.");
      send_message(sock_handle, batch);
    }
  }

  fclose(fp);

  StingerBatch batch;
  batch.set_make_undirected(true);
  batch.set_type(NUMBERS_ONLY);
  batch.set_keep_alive(false);
  send_message(sock_handle, batch);

  return 0;
}

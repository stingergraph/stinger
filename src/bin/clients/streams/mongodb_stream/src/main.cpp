#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <tclap/CmdLine.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_utils/timer.h"
#include "stinger_utils/stinger_sockets.h"
#include "stinger_net/proto/stinger-batch.pb.h"
#include "stinger_net/send_rcv.h"

#define MONGO_HAVE_STDINT
#include "mongo_c_driver/mongo.h"
#include "mongodb_stream.h"

using namespace gt::stinger;


int
main(int argc, char *argv[])
{
  /* global options */
  int stinger_port;
  int mongo_port;
  int batch_size;
  int num_batches;
  struct hostent * server = NULL;
  const char * mongo_server = NULL;
  const char * mongo_collection = NULL;

  try {
    /* parse command line configuration */
    TCLAP::CmdLine cmd("STINGER MongoDB Edge Import Stream", ' ', "1.0");
    
    TCLAP::ValueArg<std::string> stingerHostArg ("a", "host", "STINGER Server hostname", false, "localhost", "hostname", cmd);
    TCLAP::ValueArg<int> stingerPortArg ("p", "port", "STINGER Stream Port", false, 10102, "port", cmd);
    TCLAP::ValueArg<std::string> mongoHostArg ("", "mongohost", "MongoDB hostname", false, "localhost", "hostname", cmd);
    TCLAP::ValueArg<int> mongoPortArg ("", "mongoport", "MongoDB port", false, 27017, "port", cmd);
    TCLAP::ValueArg<int> batchArg ("x", "batchsize", "Number of edges per batch", false, 1000, "edges", cmd);
    TCLAP::ValueArg<int> numBatchesArg ("y", "numbatches", "Number of batches to send", false, -1, "batches", cmd);
    TCLAP::UnlabeledValueArg<std::string> collectionArg ("collection", "MongoDB database.collection (in that form)", true, "", "db.collection", cmd);

    cmd.parse (argc, argv);

    stinger_port = stingerPortArg.getValue();
    batch_size = batchArg.getValue();
    num_batches = numBatchesArg.getValue();
    
    mongo_server = mongoHostArg.getValue().c_str();
    mongo_port = mongoPortArg.getValue();
    mongo_collection = collectionArg.getValue().c_str();

    server = gethostbyname(stingerHostArg.getValue().c_str());
    if(NULL == server) {
      LOG_E_A("Hostname %s could not be resolved.", stingerHostArg.getValue().c_str());
      exit(-1);
    }
    
  } catch (TCLAP::ArgException &e)
  { std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; return 0; }


  /* Begin Mongo Server Connect */
  mongo conn[1];

  if (mongo_collection == NULL) {
    LOG_E ("Did not specify a valid MongoDB collection");
    exit(-1);
  }

  if (mongo_server == NULL) {
    LOG_E ("MongoDB server hostname is NULL");
    exit(-1);
  }

  LOG_D_A ("Mongo Server Address: %s", mongo_server);
  int status = mongo_client (conn, mongo_server, mongo_port);

  if (status != MONGO_OK) {
    switch (conn->err) {
      case MONGO_CONN_NO_SOCKET:  printf( "no socket\n" ); return 1;
      case MONGO_CONN_FAIL:       printf( "connection failed\n" ); return 1;
      case MONGO_CONN_NOT_MASTER: printf( "not master\n" ); return 1;
    }
  }
  /* End Mongo */

  /* Begin STINGER Server Connect */
  int sock_handle = connect_to_batch_server (server, stinger_port);
  LOG_V_A("Connected to %s on port %d", server->h_name, port);
  /* End STINGER */

  /* actually generate and send the batches */
  char * buf = NULL, ** fields = NULL;
  uint64_t bufSize = 0, * lengths = NULL, fieldsSize = 0, count = 0;
  int64_t line = 0;
  int batch_num = 0;
  int skip_num = 0;

  while(1) {
    StingerBatch batch;
    batch.set_make_undirected(true);
    batch.set_type(STRINGS_ONLY);
    batch.set_keep_alive(true);

    /* Begin Mongo Query */
    bson query;
    mongo_cursor cursor;

    bson_init (&query);
      bson_append_start_object (&query, "$query");
	bson_append_start_object (&query, "postedTime");
	  bson_append_bool (&query, "$exists", 1);
	bson_append_finish_object (&query);
      bson_append_finish_object (&query);

      bson_append_start_object (&query, "$orderby");
	bson_append_int (&query, "postedTime", 1);
      bson_append_finish_object (&query);
    bson_finish (&query);

    //mongo_cursor_init (&cursor, conn, "twitter.imagine");
    mongo_cursor_init (&cursor, conn, mongo_collection);
    mongo_cursor_set_query (&cursor, &query);
    cursor.limit = batch_size;
    cursor.skip = skip_num;

    int64_t print_header = 1;
    while (mongo_cursor_next(&cursor) == MONGO_OK) {
      const char * source_vtx;
      skip_num++;   // this helps us recover if we get a partial batch from Mongo

      /* "postedTime" tells us when this occured (as a string) */
      if (print_header) {
	MONGO_TOPLEVEL_OBJECT_FIND(cursor, "postedTime") 
	  printf("%s\n", bson_iterator_string(MONGO_OBJECT));

	print_header = 0;
      }

      /* "actor.preferredUsername" is the source vertex */
      MONGO_TOPLEVEL_OBJECT_FIND_BEGIN(cursor, "actor") {
	MONGO_SUBOBJECT_FIND_BEGIN("preferredUsername") {
	  //printf( "preferredUsername: %s\n", bson_iterator_string(MONGO_OBJECT));
	  source_vtx = bson_iterator_string(MONGO_OBJECT);
	} MONGO_SUBOBJECT_FIND_END();
      } MONGO_TOPLEVEL_OBJECT_FIND_END();

      /* "twitter_entities.user_mentions[].screen_name" is the destination vertex */
      MONGO_TOPLEVEL_OBJECT_FIND_BEGIN(cursor, "twitter_entities") {
	MONGO_SUBOBJECT_FORALL_BEGIN("user_mentions") {
	  const char * dest_vtx;
	  MONGO_SUBOBJECT_FIND_BEGIN("screen_name") {
	    //printf( "\tmentions %s\n", bson_iterator_string(MONGO_OBJECT));
	    dest_vtx = bson_iterator_string(MONGO_OBJECT);

	    line++;
	    EdgeInsertion * insertion = batch.add_insertions();
	    insertion->set_source_str (source_vtx);
	    insertion->set_destination_str (dest_vtx);
	    insertion->set_weight(1);
	    insertion->set_time(line);
	  } MONGO_SUBOBJECT_FIND_END();
	} MONGO_SUBOBJECT_FORALL_END();
      } MONGO_TOPLEVEL_OBJECT_FIND_END();
    }

    batch_num++;

    mongo_cursor_destroy (&cursor);
    /* End Mongo Query */

    send_message(sock_handle, batch);
    sleep(2);

    if((batch_num >= num_batches) && (num_batches != -1)) {
      break;
    }
  }
  
  /* Close connection to MongoDB */
  mongo_destroy( conn );

  /* Send the final STINGER batch */
  StingerBatch batch;
  batch.set_make_undirected(true);
  batch.set_type(STRINGS_ONLY);
  batch.set_keep_alive(false);
  //send_message(sock_handle, batch);

  return 0;
}

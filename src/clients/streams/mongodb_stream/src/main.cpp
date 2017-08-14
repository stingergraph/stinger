#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <time.h>
#include <netdb.h>
#include <unistd.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_utils/timer.h"
#include "stinger_net/proto/stinger-batch.pb.h"
#include "stinger_net/send_rcv.h"

#define MONGO_HAVE_STDINT
#include "mongo_c_driver/mongo.h"
#include "mongodb_stream.h"

using namespace gt::stinger;

#define STINGER 1

int
main(int argc, char *argv[])
{
  /* global options */
  int stinger_port = 10102;
  int mongo_port = 13001;
  int batch_size = 100000;
  int num_batches = -1;
  char const * stinger_hostname = NULL;
  char mongo_server[256];
  mongo_server[0] = '\0';

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:x:y:a:m:n:"))) {
    switch(opt) {
      case 'p': {
	stinger_port = atoi(optarg);
      } break;

      case 'x': {
	batch_size = atol(optarg);
      } break;

      case 'y': {
	num_batches = atol(optarg);
      } break;

      case 'a': {
	stinger_hostname = optarg;
      } break;

      case 'm': {
	strncpy (mongo_server, optarg, 255);
      } break;

      case 'n': {
	mongo_port = atoi(optarg);
      } break;

      case '?':
      case 'h': {
	printf("Usage:    %s [-a stinger_addr] [-p stinger_port] [-x batch_size] [-y num_batches] [-m mongo_addr] [-n mongo_port] db_name.collection_name\n", argv[0]);
	printf("Defaults:\n\tstinger_port: %d\n\tstinger_server: localhost\n\tmongo_port: %d\n\tmongo_server: localhost\n", stinger_port, mongo_port);
	exit(0);
      } break;
    }
  }

  int64_t mongo_collection_index = 0;
  if (optind < argc) {
    mongo_collection_index = optind;
  } else {
    LOG_E ("Did not specify a valid MongoDB collection");
    exit(-1);
  }

  /* Begin Mongo Server Connect */
  mongo conn[1];

  /* Default to localhost if unspecified */
  if ('\0' == mongo_server[0]) {
    strncpy (mongo_server, "127.0.0.1\0", 10);
  }

  LOG_D_A ("Mongo Server Address: %s", mongo_server);
  int status = mongo_client (conn, mongo_server, mongo_port);

  if (status != MONGO_OK) {
    switch (conn->err) {
      case MONGO_CONN_NO_SOCKET:  printf( "no socket\n" ); return 1;
      case MONGO_CONN_FAIL:       printf( "connection failed\n" ); return 1;
      case MONGO_CONN_NOT_MASTER: printf( "not master\n" ); return 1;
      default:                    printf( "unhandled error\n" ); return 1;
    }
  }
  /* End Mongo */

#if STINGER
  /* Begin STINGER Server Connect */
  if(NULL == stinger_hostname) {
    stinger_hostname = "localhost";
  }

  int sock_handle = connect_to_server (stinger_hostname, stinger_port);
  if (sock_handle == -1) exit(-1);
  /* End STINGER */

#endif
  /* actually generate and send the batches */
  /*char * buf = NULL, ** fields = NULL;
  uint64_t bufSize = 0, * lengths = NULL, fieldsSize = 0, count = 0;*/
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
    mongo_cursor_init (&cursor, conn, argv[mongo_collection_index]);
    mongo_cursor_set_query (&cursor, &query);
    cursor.limit = batch_size;
    cursor.skip = skip_num;
    //printf("limit = %ld\n", cursor.limit);
    //printf("skip = %ld\n", cursor.skip);

    int64_t print_header = 1;
    while (mongo_cursor_next(&cursor) == MONGO_OK) {
      const char * source_vtx = NULL;
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

#if STINGER
    send_message(sock_handle, batch);
#endif
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

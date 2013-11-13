#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_net/mon_handling.h"
#include "stinger_net/stinger_mon.h"
#include "stinger_utils/timer.h"

#define MONGO_HAVE_STDINT
#include "mongo_c_driver/mongo.h"
#include "alg_to_mongo.h"

#define BSON_BLOCKSIZE 2000

using namespace gt::stinger;


int main (int argc, char *argv[])
{
  /* global options */
  int mongo_port = 13001;
  char mongo_server[256];
  mongo_server[0] = '\0';
  char collection_name[256];

  init_timer();

  int opt = 0;
  while (-1 != (opt = getopt(argc, argv, "m:n:"))) {
    switch (opt) {
      case 'm': {
	strncpy (mongo_server, optarg, 255);
      } break;

      case 'n': {
	mongo_port = atoi(optarg);
      } break;

      case '?':
      case 'h': {
	printf("Usage:    %s [-m mongo_addr] [-n mongo_port] algorithm_name\n", argv[0]);
	printf("Defaults:\n\tmongo_port: %d\n\tmongo_server: localhost\n", mongo_port);
	exit(0);
      } break;
    }
  }

  int64_t algorithm_name_index = 0;
  if (optind < argc) {
    algorithm_name_index = optind;
  } else {
    LOG_E ("Did not specify a valid algorithm");
    exit(-1);
  }

  snprintf (collection_name, 255, "stinger.%s", argv[algorithm_name_index]);
  LOG_D_A ("Output Collection Name: %s", collection_name);

  /* Connect to MongoDB */
  mongo conn;
  
  /* Default to localhost if unspecified */
  if ('\0' == mongo_server[0]) {
    strncpy (mongo_server, "127.0.0.1\0", 10);
  }

  LOG_D_A ("Mongo Server Address: %s", mongo_server);
  int status = mongo_client (&conn, mongo_server, mongo_port);

  if (status != MONGO_OK) {
    switch (conn.err) {
      case MONGO_CONN_NO_SOCKET:  printf( "no socket\n" ); return 1;
      case MONGO_CONN_FAIL:       printf( "connection failed\n" ); return 1;
      case MONGO_CONN_NOT_MASTER: printf( "not master\n" ); return 1;
    }
  }
  /* End MongoDB connect */

  /* Connect to STINGER as a monitor */
  if (-1 == mon_connect(10103, "localhost", "alg_to_mongo")) {
    LOG_E_A ("Connect to STINGER on %s:%ld failed", "localhost", 10103);
    return -1;
  }

  StingerMon & mon = StingerMon::get_mon();
  //int64_t num_algs = mon.get_num_algs();
  //printf("num_algs = %ld\n", (long) num_algs);
  /* End STINGER connect */
    
  /* initialize BSON objects */
  bson ** ps = (bson **) xmalloc (BSON_BLOCKSIZE * sizeof(bson *));
  for (int64_t i = 0; i < BSON_BLOCKSIZE; i++) {
    ps[i] = (bson *) xmalloc (sizeof(bson));
  }

  while (1)
  {
    int64_t cur_time = get_current_timestamp();
    
    mon.wait_for_sync();
    LOG_D_A ("Processing timestamp %ld", cur_time);
    tic();

    /* Get the STINGER pointer -- critical section */
    mon.get_alg_read_lock();

    stinger_t * S = mon.get_stinger();
    if (!S) {
      LOG_E ("bad stinger pointer");
      return -1;
    }

    /* Get a pointer to the AlgState */
    StingerAlgState * alg_state = mon.get_alg(argv[algorithm_name_index]);

    if (!alg_state) {
      mon.release_alg_read_lock();
      LOG_E_A ("Algorithm %s does not exist", argv[algorithm_name_index]);
      exit(-1);
    }
	  
    int64_t nv = stinger_mapping_nv(S);

    for (int64_t block = 0; block < nv; block += BSON_BLOCKSIZE)
    {
      int64_t end = block + BSON_BLOCKSIZE;
      if (end > nv)
	end = nv;
	  
      /* Convert the algorithm data array to BSON */
      int64_t count = array_to_bson   (ps, S, nv, alg_state, cur_time, block, end);
	  
      /* Mongo send batch */
      if (MONGO_ERROR == mongo_insert_batch (&conn, collection_name, (const bson **)ps, count, 0, 0)) {
	LOG_E ("MongoDB insert failed");
	LOG_E_A ("conn->err: %ld", conn.err);
      }
    }



    /* Release the lock */
    mon.release_alg_read_lock();
  
    double time = toc();
    LOG_I_A ("elapsed: %20.15e sec", time);
    LOG_I_A ("nv: %ld", nv);
    LOG_I_A ("rate: %20.15e vtx/sec", (double) nv / time);


  } /* end infinite loop */
	  
  
  /* Cleanup */
  for (int64_t i = 0; i < BSON_BLOCKSIZE; i++) {
    bson_destroy (ps[i]);
    free (ps[i]);
  }

  return 0;
}



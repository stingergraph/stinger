#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "stinger_net/mon_handling.h"
#include "stinger_net/stinger_mon.h"
#include "stinger_utils/timer.h"

using namespace gt::stinger;


int main (int argc, char *argv[])
{
  char filename[256] = "dumped_graph.txt";

  /* global options */
  init_timer();

  int opt = 0;
  while (-1 != (opt = getopt(argc, argv, "f:"))) {
    switch (opt) {
      case 'f': {
	strncpy (filename, optarg, 255);
      } break;

      case '?':
      case 'h': {
	printf("Usage:    %s [-f filename]\n", argv[0]);
	printf("Defaults:\n\tfilename: %s\n", filename);
	exit(0);
      } break;
    }
  }

  /* Connect to STINGER as a monitor */
  if (-1 == mon_connect(10103, "localhost", "dump_graph_to_disk")) {
    LOG_E_A ("Connect to STINGER on %s:%d failed", "localhost", 10103);
    return -1;
  }

  StingerMon & mon = StingerMon::get_mon();
  /* End STINGER connect */
 
  /* Open the output file */
  FILE *fp = fopen(filename, "w");
  if (!fp) {
    LOG_E("File could not be opened for writing");
    exit(-1);
  }

  //mon.wait_for_sync();
  LOG_I("Begin writing to disk");

  tic();

  /* Get the STINGER pointer -- critical section */
  stinger_t * S = NULL;

  /* TODO: there is a race condition handled here that should be handled by the library */
  while (!S) {
    mon.get_alg_read_lock();
    S = mon.get_stinger();
    if (!S) {
      mon.release_alg_read_lock();
    }
    usleep(100);
  }
 
  if (!S) {
    LOG_E ("bad stinger pointer");
    return -1;
  }

  int64_t nv = S->max_nv;
  int64_t ne = 0;

  for (int64_t i = 0; i < nv; i++) {
    STINGER_FORALL_EDGES_OF_VTX_BEGIN(S, i) {
      fprintf(fp, "%ld %ld %ld %ld %ld %ld\n",  STINGER_EDGE_SOURCE, 
					    STINGER_EDGE_DEST,
					    STINGER_EDGE_TYPE,
					    STINGER_EDGE_WEIGHT,
					    STINGER_EDGE_TIME_FIRST,
					    STINGER_EDGE_TIME_RECENT);
      ne++;
    } STINGER_FORALL_EDGES_OF_VTX_END();
  }

  /* Release the lock */
  mon.release_alg_read_lock();

  double time = toc();
  LOG_I_A ("elapsed: %20.15e sec", time);
  LOG_I_A ("nv: %ld", nv);
  LOG_I_A ("ne: %ld", ne);
  LOG_I_A ("rate: %20.15e edges/sec", (double) ne / time);


	  
  
  /* Cleanup */
  fclose(fp);
  return 0;
}



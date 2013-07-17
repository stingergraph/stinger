#include <cstdio>
#include <algorithm>
#include <limits>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>

extern "C" {
#include "stinger_core/stinger.h"
#include "stinger_core/stinger_shared.h"
#include "stinger_core/xmalloc.h"
#include "stinger_utils/stinger_utils.h"
#include "stinger_utils/timer.h"
#include "stinger_utils/dimacs_support.h"
#include "stinger_utils/csv.h"
}

#include "proto/stinger-batch.pb.h"
#include "server.h"
#include "send_rcv.h"

using namespace gt::stinger;

int main(int argc, char *argv[])
{
  /* default global options */
  int port = 10101;
  uint64_t buffer_size = 1ULL << 28ULL;
  char * graph_name = (char *) xmalloc (128*sizeof(char));
  sprintf(graph_name, "/default");
  char * input_file = (char *) xmalloc (1024*sizeof(char));
  input_file[0] = '\0';
  char * file_type = (char *) xmalloc (128*sizeof(char));
  file_type[0] = '\0';

  /* parse command line configuration */
  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:b:n:i:t:h?"))) {
    switch(opt) {
      case 'p': {
		  port = atoi(optarg);
		} break;

      case 'b': {
		  buffer_size = atol(optarg);
		} break;

      case 'n': {
		  strcpy (graph_name, optarg);
		} break;

      case 'i': {
		  strcpy (input_file, optarg);
		} break;
      
      case 't': {
		  strcpy (file_type, optarg);
		} break;

      case '?':
      case 'h': {
		  printf("Usage:    %s [-p port] [-b buffer_size] [-n graph_name] [-i input_file_path [-t file_type]]\n", argv[0]);
		  printf("Defaults:\n\tport: %d\n\tbuffer_size: %lu\n\tgraph_name: %s\n", port, (unsigned long) buffer_size, graph_name);
		  exit(0);
		} break;

    }
  }

  /* print configuration to the terminal */
  printf("\tName: %s\n", graph_name);

  /* allocate the graph */
  struct stinger * S = stinger_shared_new(&graph_name);
  //struct stinger * S = stinger_new ();
  size_t graph_sz = S->length + sizeof(struct stinger);


  /* load edges from disk (if applicable) */
  if (input_file[0] != '\0')
  {
    switch (file_type[0])
    {
      case 'b': {
		  int64_t nv, ne;
		  int64_t *off, *ind, *weight, *graphmem;
		  snarf_graph (input_file, &nv, &ne, (int64_t**)&off,
		      (int64_t**)&ind, (int64_t**)&weight, (int64_t**)&graphmem);
		  stinger_set_initial_edges (S, nv, 0, off, ind, weight, NULL, NULL, 0);
		  free(graphmem);
		} break;  /* STINGER binary */

      case 'c': {
		  load_csv_graph (S, input_file, 1);
		} break;  /* CSV */

      case 'd': {
		  load_dimacs_graph (S, input_file);
		} break;  /* DIMACS */

      case 'e': {
		} break;  /* Edge list */

      case 'g': {
		} break;  /* GML / GraphML / GEXF -- you pick */

      case 'j': {
		} break;  /* JSON */

      case 'x': {
		} break;  /* XML */

      default:	{
		  printf("Unsupported file type.\n");
		  exit(0);
		} break;
    }
  }


  printf("Graph created. Running stats...");
  tic();
  printf("\n\tVertices: %ld\n\tEdges: %ld\n",
      stinger_num_active_vertices(S), stinger_total_edges(S));

  /* consistency check */
  printf("\tConsistency %ld\n", stinger_consistency_check(S, STINGER_MAX_LVERTICES));
  printf("\tDone. %lf seconds\n", toc());

  /* we need a socket that can reply with the shmem name & size of the graph */
  pid_t name_pid, batch_pid;

  /* child will handle name and size requests */
  name_pid = fork ();
  if (name_pid == 0)
  {
    start_udp_graph_name_server (graph_name, graph_sz, port);
    exit (0);
  }

  /* and we need a listener that can receive new edges */
  batch_pid = fork ();
  if (batch_pid == 0)
  {
    start_tcp_batch_server (S, port, buffer_size);
    exit (0);
  }

  printf("Press <return> to shut down the server...\n");
  getchar();

  printf("Shutting down the name server..."); fflush(stdout);
  int status;
  kill(name_pid, SIGTERM);
  waitpid(name_pid, &status, 0);
  printf(" done.\n"); fflush(stdout);

  printf("Shutting down the batch server..."); fflush(stdout);
  kill(batch_pid, SIGTERM);
  waitpid(batch_pid, &status, 0);
  printf(" done.\n"); fflush(stdout);

  /* clean up */
  stinger_shared_free(S, graph_name, graph_sz);
  free(graph_name);
  free(input_file);

  return 0;
}

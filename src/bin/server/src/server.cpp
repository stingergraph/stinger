#include <cstdio>
#include <algorithm>
#include <limits>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

#include "server.h"
#include "stinger_net/stinger_server_state.h"

extern "C" {
#include "stinger_core/stinger.h"
#include "stinger_core/stinger_shared.h"
#include "stinger_core/xmalloc.h"
#include "stinger_utils/stinger_utils.h"
#include "stinger_utils/timer.h"
#include "stinger_utils/dimacs_support.h"
#include "stinger_utils/metisish_support.h"
#include "stinger_utils/json_support.h"
#include "stinger_utils/csv.h"
}

using namespace gt::stinger;

static char * graph_name = NULL;
static char * input_file = NULL;
static char * file_type = NULL;
static bool save_to_disk = false;

static int start_pipe[2] = {-1, -1};

/* we need a socket that can reply with the shmem name & size of the graph */
static pthread_t name_pid, batch_pid;

static StingerServerState & server_state = StingerServerState::get_server_state();

static void cleanup (void);
extern "C" {
  static void sigterm_cleanup (int);
}
  
int main(int argc, char *argv[])
{
  /* default global options */
  int port_names = 10101;
  int port_streams = port_names + 1;
  int port_algs = port_names + 2;
  int unleash_daemon = 0;

  graph_name = (char *) xmalloc (128*sizeof(char));
  sprintf(graph_name, "/stinger-default");
  input_file = (char *) xmalloc (1024*sizeof(char));
  input_file[0] = '\0';
  file_type = (char *) xmalloc (128*sizeof(char));
  file_type[0] = '\0';
  int use_numerics = 0;

  /* parse command line configuration */
  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "a:s:p:b:n:i:t:1h?dkvc:f:"))) {
    switch(opt) {
      case 'p': {
		  port_names = atoi(optarg);
		} break;

      case 'a': {
		  port_algs = atoi(optarg);
		} break;
      case 's': {
		  port_streams = atoi(optarg);
		} break;

      case 'n': {
		  strcpy (graph_name, optarg);
		} break;
      
      case 'k': {
		  server_state.set_write_alg_data(true);
		} break;

      case 'v': {
		  server_state.set_write_names(true);
		} break;

      case 'c': {
		  server_state.set_history_cap(atol(optarg));
		} break;

      case 'f': {
		  server_state.set_out_dir(optarg);
		} break;

      case 'i': {
		  strcpy (input_file, optarg);
		} break;
      
      case 't': {
		  strcpy (file_type, optarg);
		} break;

      case '1': {
		  use_numerics = 1;
		} break;
      case 'd': {
		  unleash_daemon = 1;
		} break;

      case '?':
      case 'h': {
		  printf("Usage:    %s\n"
		         "   [-p port_names]\n"
			 "   [-a port_algs]\n"
			 "   [-s port_streams]\n"
			 "   [-n graph_name]\n"
			 "   [-i input_file_path]\n"
			 "   [-t file_type]\n"
			 "   [-1 (for numeric IDs)]\n"
			 "   [-d daemon mode]\n"
			 "   [-k write algorithm states to disk]\n"
			 "   [-v write vertex name mapping to disk]\n"
			 "   [-f output directory for vertex names, alg states]\n"
			 "   [-c cap number of history files to keep per alg]  \n", argv[0]);
		  printf("Defaults:\n\tport_names: %d\n\tport_algs: %d\n\tport_streams: %d\n\tgraph_name: %s\n", port_names, port_algs, port_streams, graph_name);
		  exit(0);
		} break;

    }
  }

  /* print configuration to the terminal */
  printf("\tName: %s\n", graph_name);

  /* If being a "daemon" (after a fashion), wait on the child to finish initializing and then exit. */
  if (unleash_daemon) {
    pid_t pid;
    if (pipe (start_pipe)) {
      perror ("pipe");
      abort ();
    }
    pid = fork ();
    if (pid < 0) {
      perror ("fork");
      abort ();
    }
    if (0 != pid) { /* parent */
      int exitcode;
      close (start_pipe[1]);
      read (start_pipe[0], &exitcode, sizeof (exitcode));
      printf ("Server running.\n");
      close (start_pipe[0]);
      return exitcode;
    }
    /* else */
    setsid (); /* XXX: Someday look for errors and abort horribly... */
    close (start_pipe[0]);
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset (&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sigaction (SIGHUP, &sa, NULL); /* Paranoia, should no longer be attached to the tty */
  }

  /* allocate the graph */
  struct stinger * S = stinger_shared_new(&graph_name);
  size_t graph_sz = S->length + sizeof(struct stinger);

  /* load edges from disk (if applicable) */
  if (input_file[0] != '\0')
  {
    printf("\tReading...");
    tic ();
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
		  load_csv_graph (S, input_file, use_numerics);
		} break;  /* CSV */

      case 'd': {
		  load_dimacs_graph (S, input_file);
		} break;  /* DIMACS */

      case 'm': {
		  load_metisish_graph (S, input_file);
		} break;

      case 'g': {
		} break;  /* GML / GraphML / GEXF -- you pick */

      case 'j': {
		  load_json_graph (S, input_file);
		} break;  /* JSON */

      case 'x': {
		} break;  /* XML */

      case 'r': {
		  uint64_t nv;
		  stinger_open_from_file(input_file, S, &nv);
		  save_to_disk = true;
		} break;  /* restartable STINGER on disk */

      default:	{
		  printf("Unsupported file type.\n");
		  exit(0);
		} break;
    }
    printf(" (done: %lf s)\n", toc ());
  }


  printf("Graph created. Running stats...");
  tic();
  printf("\n\tVertices: %ld\n\tEdges: %ld\n",
      stinger_num_active_vertices(S), stinger_total_edges(S));

  /* consistency check */
  printf("\tConsistency %ld\n", (long) stinger_consistency_check(S, S->max_nv));
  printf("\tDone. %lf seconds\n", toc());

  /* initialize the singleton members */
  server_state.set_stinger(S);
  server_state.set_stinger_loc(graph_name);
  server_state.set_stinger_sz(graph_sz);
  server_state.set_port(port_names, port_streams, port_algs);
  server_state.set_mon_stinger(graph_name, sizeof(stinger_t) + S->length);
 
  /* this thread will handle the shared memory name mapping */
  pthread_create (&name_pid, NULL, start_udp_graph_name_server, NULL);

  /* this thread will handle the batch & alg servers */
  /* TODO: bring the thread creation for the alg server to this level */
  pthread_create (&batch_pid, NULL, start_tcp_batch_server, NULL);

  {
    /* Inform the parent that we're ready for connections. */
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset (&sa.sa_mask);
    sa.sa_handler = sigterm_cleanup;
    /* Ignore the old handlers. */
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);
    sigaction (SIGHUP, &sa, NULL);
  }

  if(unleash_daemon) {
    int exitcode = EXIT_SUCCESS;
    write (start_pipe[1], &exitcode, sizeof (exitcode));
    close (start_pipe[1]);
    while(1) { sleep(10); }
  } else {
    printf("Press <q> to shut down the server...\n");
    while (getchar() != 'q');
  }

  cleanup ();

  return 0;
}

void
cleanup (void)
{
  printf("Shutting down the name server..."); fflush(stdout);
  int status;
  kill(name_pid, SIGTERM);
  waitpid(name_pid, &status, 0);
  printf(" done.\n"); fflush(stdout);

  printf("Shutting down the batch server..."); fflush(stdout);
  kill(batch_pid, SIGTERM);
  waitpid(batch_pid, &status, 0);
  printf(" done.\n"); fflush(stdout);

  struct stinger * S = server_state.get_stinger();
  size_t graph_sz = S->length + sizeof(struct stinger);
  
  printf("\tConsistency %ld\n", (long) stinger_consistency_check(S, S->max_nv));

  /* snapshot to disk */
  if (save_to_disk) {
    stinger_save_to_file(S, stinger_max_active_vertex(S) + 1, input_file);
  }

  /* clean up */
  stinger_shared_free(S, graph_name, graph_sz);
  shmunlink(graph_name);
  free(graph_name);
  free(input_file);
  free(file_type);

  /* clean up algorithm data stores */
  for (size_t i = 0; i < server_state.get_num_algs(); i++) {
    StingerAlgState * alg_state = server_state.get_alg(i);
    const char * alg_data_loc = alg_state->data_loc.c_str();
    shmunlink(alg_data_loc);
  }
}

void
sigterm_cleanup (int)
{
  cleanup ();
  exit (EXIT_SUCCESS);
}

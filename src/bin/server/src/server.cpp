#include <cstdio>
#include <algorithm>
#include <limits>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <tclap/CmdLine.h>

#include "server.h"
#include "stinger_net/stinger_server_state.h"

extern "C" {
#include "stinger_core/stinger.h"
#include "stinger_core/stinger_shared.h"
#include "stinger_core/xmalloc.h"
#include "stinger_utils/stinger_utils.h"
#include "stinger_utils/timer.h"
#include "stinger_utils/dimacs_support.h"
#include "stinger_utils/json_support.h"
#include "stinger_utils/csv.h"
}

using namespace gt::stinger;

int main(int argc, char *argv[])
{
  StingerServerState & server_state = StingerServerState::get_server_state();


  try {
    /* parse command line configuration */
    TCLAP::CmdLine cmd("Welcome to STINGER", ' ', "2.0");

    TCLAP::ValueArg<int> portArg ("p", "port", "Base TCP Port", false, 10101, "port");
    cmd.add (portArg);

    TCLAP::ValueArg<std::string> graphNameArg ("n", "name", "Graph Name", false, "/default", "name");
    cmd.add (graphNameArg);

    TCLAP::SwitchArg algDataSwitch ("", "saveAlgState", "Write algorithm states to disk after each batch of updates", cmd, false);
    TCLAP::SwitchArg writeNamesSwitch ("m", "saveVertexMap", "Write vertex name mapping to disk", cmd, false);
    TCLAP::SwitchArg numericSwitch ("1", "numeric", "Use numeric IDs", cmd, false);
    TCLAP::SwitchArg daemonSwitch ("d", "daemon", "Daemon mode", cmd, false);

    TCLAP::ValueArg<int> historyCapArg ("c", "limit", "Cap the number of history files to keep per algorithm", false, 0, "number");
    cmd.add (historyCapArg);

    TCLAP::ValueArg<std::string> outDirArg ("f", "out", "Output directory for vertex names, algorithm states", false, "", "path");
    cmd.add (outDirArg);

    TCLAP::ValueArg<std::string> inputFileArg ("i", "input", "Input filename", false, "", "filename");
    cmd.add (inputFileArg);

    TCLAP::ValueArg<std::string> fileTypeArg ("t", "type", "Input file type", false, "", "type");
    cmd.add (fileTypeArg);

    cmd.parse (argc, argv);




    /* default global options */
    int port_names = portArg.getValue();
    int port_streams = port_names + 1;
    int port_algs = port_names + 2;

    std::string graph_name_str = graphNameArg.getValue();
    std::string input_file = inputFileArg.getValue();
    std::string file_type = fileTypeArg.getValue();
    bool use_numerics = numericSwitch.getValue();
    bool unleash_daemon = daemonSwitch.getValue();

    char * graph_name = (char *) xmalloc (128*sizeof(char));
    strncpy (graph_name, graph_name_str.c_str(), 127);
    graph_name[127] = '\0';

    if (algDataSwitch.getValue()) {
      server_state.set_write_alg_data(true);
    }

    if (writeNamesSwitch.getValue()) {
      server_state.set_write_names(true);
    }

    int history_cap = historyCapArg.getValue();
    if (history_cap > 0) {
      server_state.set_history_cap(history_cap);
    }

    std::string out_dir = outDirArg.getValue();
    if (out_dir != "") {
      server_state.set_out_dir(optarg);
    }


    /* print configuration to the terminal */
    printf("\tName: %s\n", graph_name);

    /* allocate the graph */
    struct stinger * S = stinger_shared_new(&graph_name);
    size_t graph_sz = S->length + sizeof(struct stinger);


    /* load edges from disk (if applicable) */
#if 0
    if (input_file == "")
    {
      switch (file_type)
      {
	case 'b': {
		    int64_t nv, ne;
		    int64_t *off, *ind, *weight, *graphmem;
		    snarf_graph (input_file.c_str(), &nv, &ne, (int64_t**)&off,
			(int64_t**)&ind, (int64_t**)&weight, (int64_t**)&graphmem);
		    stinger_set_initial_edges (S, nv, 0, off, ind, weight, NULL, NULL, 0);
		    free(graphmem);
		  } break;  /* STINGER binary */

	case 'c': {
		    load_csv_graph (S, input_file.c_str(), use_numerics);
		  } break;  /* CSV */

	case 'd': {
		    load_dimacs_graph (S, input_file.c_str());
		  } break;  /* DIMACS */

	case 'g': {
		  } break;  /* GML / GraphML / GEXF -- you pick */

	case 'j': {
		    load_json_graph (S, input_file.c_str());
		  } break;  /* JSON */

	case 'x': {
		  } break;  /* XML */

	default:	{
		    printf("Unsupported file type.\n");
		    exit(0);
		  } break;
      }
    }
#endif


    printf("Graph created. Running stats...");
    tic();
    printf("\n\tVertices: %ld\n\tEdges: %ld\n",
	stinger_num_active_vertices(S), stinger_total_edges(S));

    /* consistency check */
    printf("\tConsistency %ld\n", (long) stinger_consistency_check(S, S->max_nv));
    printf("\tDone. %lf seconds\n", toc());

    /* we need a socket that can reply with the shmem name & size of the graph */
    pid_t name_pid, batch_pid;

    /* child will handle name and size requests */
    name_pid = fork ();
    if (name_pid == 0)
    {
      start_udp_graph_name_server (graph_name, graph_sz, port_names);
      exit (0);
    }

    /* and we need a listener that can receive new edges */
    batch_pid = fork ();
    if (batch_pid == 0)
    {
      start_tcp_batch_server (S, graph_name, port_streams, port_algs);
      exit (0);
    }

    if(unleash_daemon) {
      while(1) { sleep(10); }
    } else {
      printf("Press <q> to shut down the server...\n");
      while (getchar() != 'q');
    }

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

    if (graph_name)
      free(graph_name);

  
  } catch (TCLAP::ArgException &e)
  { std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; }
  
  return 0;
}

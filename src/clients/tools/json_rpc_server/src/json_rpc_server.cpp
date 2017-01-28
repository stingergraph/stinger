#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <functional>
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "mongoose/mongoose.h"
#include "stinger_utils/timer.h"
#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "json_rpc_server.h"
#include "json_rpc.h"
#include "rpc_state.h"
#include "mon_handling.h"
#include "session_handling.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


static int
send_response(struct mg_connection *conn, rapidjson::Document& response)
{
  /* format the response */
  rapidjson::StringBuffer out_buf;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(out_buf);
  response.Accept(writer);

  const char * out_ch = out_buf.GetString();
  int out_len = out_buf.Size();

  /* send the response */
  return  mg_printf(conn,
	    "HTTP/1.1 200 OK\r\n"
	    "Content-Type: text/plain\r\n"
	    "Content-Length: %d\r\n"        // Always set Content-Length
	    "Access-Control-Allow-Origin: *\r\n"
	    "\r\n"
	    "%s",
	    out_len, out_ch);
}

static int
create_error(rapidjson::Document& response)
{
  rapidjson::Value is_null;
  is_null.SetNull();
  rapidjson::Value result;
  result.SetObject();
  response.SetObject();

  rapidjson::Document::AllocatorType& allocator = response.GetAllocator();

  response.AddMember("jsonrpc", "2.0", allocator);
  json_rpc_error (-32700, result, allocator);
  response.AddMember("error", result, allocator);
  response.AddMember("id", is_null, allocator);
 
  return 0;
}

#define MAX_REQUEST_SIZE (1ULL << 22ULL)

static int
begin_request_handler(struct mg_connection *conn)
{
  init_timer();
  double time = timer();

  const struct mg_request_info *request_info = mg_get_request_info(conn);
  LOG_D_A("Receiving request for %s", request_info->uri);

  if (strncmp(request_info->uri, "/data/", 6)==0) {
    return 0;
  }

  if (strncmp(request_info->uri, "/jsonrpc", 8)==0) {
    rapidjson::Document input, output;

    /* read the incoming request into a buffer */
    uint8_t * storage = (uint8_t *) xcalloc (1, MAX_REQUEST_SIZE);

    int64_t read = mg_read(conn, storage, MAX_REQUEST_SIZE);
    if (read > ((int64_t) (MAX_REQUEST_SIZE-2))) {
      LOG_E_A("Request was too large: %" PRId64, read);
      
      /* send an error back */
      create_error(output);
      send_response(conn, output);

      free(storage);
      return -1;
    }
    storage[read] = '\0';

    /* parse the request */
    LOG_D_A("Parsing request:\n%.*s", read, storage);
    input.Parse<0>((char *)storage);
    free(storage);

    /* process the request */
    json_rpc_process_request(input, output);
    
    /* calculate execution time and append to response */
    time = timer() - time;
    double milliseconds = round(time * 1.0e3f * 100.0f) / 100.0f;
    rapidjson::Value exec_time;
    exec_time.SetDouble(milliseconds);
    rapidjson::Document::AllocatorType& allocator = output.GetAllocator();
    output.AddMember("millis", exec_time, allocator);

    return send_response(conn, output);
  }

  return 0;
}


int
main (int argc, char ** argv)
{
  int unleash_daemon = 0;

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "h?d"))) {
    switch(opt) {
      default: {
	LOG_E_A("Unknown option %c", opt);
      } /* no break */
      case '?':
      case 'h': {
	printf("Usage: %s [-d]\n", argv[0]);
	printf("-d\tdaemon mode\n");
	exit(-1);
      } break;
	
      case 'd': {
	unleash_daemon = 1;
      } break;
    }
  }

  JSON_RPCServerState & server_state = JSON_RPCServerState::get_server_state();
  server_state.add_rpc_function("get_algorithms", new JSON_RPC_get_algorithms(&server_state));
  server_state.add_rpc_function("get_data_description", new JSON_RPC_get_data_description(&server_state));
  server_state.add_rpc_function("get_data_array_range", new JSON_RPC_get_data_array_range(&server_state));
  server_state.add_rpc_function("get_data_array_sorted_range",	new JSON_RPC_get_data_array_sorted_range(&server_state));
  server_state.add_rpc_function("get_data_array", new JSON_RPC_get_data_array(&server_state));
  server_state.add_rpc_function("get_data_array_set", new JSON_RPC_get_data_array_set(&server_state));
  server_state.add_rpc_function("get_graph_stats", new JSON_RPC_get_graph_stats(&server_state));
  server_state.add_rpc_function("label_breadth_first_search", new JSON_RPC_label_breadth_first_search(&server_state));
  server_state.add_rpc_function("label_mod_expand", new JSON_RPC_label_mod_expand(&server_state));
  server_state.add_rpc_function("breadth_first_search", new JSON_RPC_breadth_first_search(&server_state));
  server_state.add_rpc_function("adamic_adar_index", new JSON_RPC_adamic_adar(&server_state));
  server_state.add_rpc_function("community_on_demand", new JSON_RPC_community_on_demand(&server_state));
  server_state.add_rpc_function("register", new JSON_RPC_register(&server_state));
  server_state.add_rpc_function("request", new JSON_RPC_request(&server_state));
  server_state.add_rpc_function("get_data_array_reduction", new JSON_RPC_get_data_array_reduction(&server_state));
  server_state.add_rpc_function("get_server_info", new JSON_RPC_get_server_info(&server_state));
  server_state.add_rpc_function("get_server_health", new JSON_RPC_get_server_health(&server_state));
  server_state.add_rpc_function("egonet", new JSON_RPC_egonet(&server_state));
  server_state.add_rpc_function("get_connected_component", new JSON_RPC_get_connected_component(&server_state));
  server_state.add_rpc_function("bfs_edges", new JSON_RPC_bfs_edges(&server_state));
  server_state.add_rpc_function("pagerank_subgraph", new JSON_RPC_pagerank_subgraph(&server_state));
  server_state.add_rpc_function("exact diameter", new JSON_RPC_exact_diameter(&server_state));
  server_state.add_rpc_function("single source shortest path", new JSON_RPC_single_source_shortest_path(&server_state));

  server_state.add_rpc_session("subgraph", new JSON_RPC_community_subgraph(0, &server_state));
  server_state.add_rpc_session("vertex_event_notifier", new JSON_RPC_vertex_event_notifier(0, &server_state));
  server_state.add_rpc_session("get_latlon", new JSON_RPC_get_latlon(0, &server_state));
  server_state.add_rpc_session("get_latlon_gnip", new JSON_RPC_get_latlon_gnip(0, &server_state));
  server_state.add_rpc_session("get_latlon_twitter", new JSON_RPC_get_latlon_twitter(0, &server_state));

  mon_connect(10103, "localhost", "json_rpc");

  /* Mongoose setup and start */
  /*struct mg_context *ctx;*/
  struct mg_callbacks callbacks;

  // List of options. Last element must be NULL.
  const char *opts[] = {"listening_ports", "8088", "document_root", "data", NULL};

  // Prepare callbacks structure. We have only one callback, the rest are NULL.
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.begin_request = begin_request_handler;

  // Start the web server.
  (void) mg_start(&callbacks, NULL, opts);
  
  /* infinite loop */
  if(unleash_daemon) {
    while(1) { sleep(10); }
  } else {
    printf("Press Ctrl-C to shut down the server...\n");
    while(1) { sleep(10); }
  }

  return 0;
}

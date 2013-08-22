#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <functional>

#define LOG_AT_W  /* warning only */

extern "C" {
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
}

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include "json_rpc_server.h"
#include "json_rpc.h"
#include "rpc_state.h"
#include "mon_handling.h"

#include "mongoose/mongoose.h"

using namespace gt::stinger;

#define MAX_REQUEST_SIZE (1ULL << 22ULL)

static int
begin_request_handler(struct mg_connection *conn)
{
  const struct mg_request_info *request_info = mg_get_request_info(conn);
  LOG_D_A("Receiving request for %s", request_info->uri);

  if (strncmp(request_info->uri, "/data/", 6)==0) {
    return 0;
  }

  if (strncmp(request_info->uri, "/jsonrpc", 8)==0) {
    uint8_t * storage = (uint8_t *) xcalloc (1, MAX_REQUEST_SIZE);

    int64_t read = mg_read(conn, storage, MAX_REQUEST_SIZE);
    if (read > MAX_REQUEST_SIZE-2) {
      LOG_E_A("Request was too large: %ld", read);
    }
    storage[read] = '\0';

    LOG_D_A("Parsing request:\n%.*s", read, storage);
    rapidjson::Document input, output;
    input.Parse<0>((char *)storage);

    json_rpc_process_request(input, output);

    rapidjson::StringBuffer out_buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(out_buf);
    output.Accept(writer);

    const char * out_ch = out_buf.GetString();
    int out_len = out_buf.Size();

    LOG_D_A("RapidJSON parsed :%d\n%s", out_len, out_ch);

    int code = mg_printf(conn,
	      "HTTP/1.1 200 OK\r\n"
	      "Content-Type: text/plain\r\n"
	      "Content-Length: %d\r\n"        // Always set Content-Length
	      "\r\n"
	      "%s",
	      out_len, out_ch);

    LOG_D_A("Code was %d", code);

    free(storage);

    return 1;
  }

  return 0;
}


int
main (void)
{
  JSON_RPCServerState & server_state = JSON_RPCServerState::get_server_state();
  server_state.add_rpc_function("get_algorithms", new JSON_RPC_get_algorithms(&server_state));
  server_state.add_rpc_function("get_data_description", new JSON_RPC_get_data_description(&server_state));
  server_state.add_rpc_function("get_data_array_range", new JSON_RPC_get_data_array_range(&server_state));
  server_state.add_rpc_function("get_data_array_sorted_range",	new JSON_RPC_get_data_array_sorted_range(&server_state));
  server_state.add_rpc_function("get_data_array", new JSON_RPC_get_data_array(&server_state));
  server_state.add_rpc_function("get_data_array_set", new JSON_RPC_get_data_array_set(&server_state));
  server_state.add_rpc_function("get_graph_stats", new JSON_RPC_get_graph_stats(&server_state));
  server_state.add_rpc_function("breadth_first_search", new JSON_RPC_breadth_first_search(&server_state));
  server_state.add_rpc_function("register", new JSON_RPC_register(&server_state));
  server_state.add_rpc_function("request", new JSON_RPC_request(&server_state));

  mon_connect(10103, "localhost", "json_rpc");

  /* Mongoose setup and start */
  struct mg_context *ctx;
  struct mg_callbacks callbacks;

  // List of options. Last element must be NULL.
  const char *opts[] = {"listening_ports", "8088", NULL};

  // Prepare callbacks structure. We have only one callback, the rest are NULL.
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.begin_request = begin_request_handler;

  // Start the web server.
  ctx = mg_start(&callbacks, NULL, opts);
  
  /* infinite loop */
  while (getchar() != 'q');

  return 0;
}

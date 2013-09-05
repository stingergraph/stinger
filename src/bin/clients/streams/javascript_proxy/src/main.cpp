#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <functional>

//#define LOG_AT_W  /* warning only */

extern "C" {
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_utils/stinger_sockets.h"
}

#include "mongoose/mongoose.h"
#include "stinger_net/proto/stinger-batch.pb.h"
#include "stinger_net/send_rcv.h"

using namespace gt::stinger;

/* global variables */
int port = 10102;
struct hostent * server = NULL;


static void websocket_ready_handler(struct mg_connection *conn) 
{
  LOG_D ("Entering websocket_ready_handler()");
  static const char *message = "server ready";
  //mg_websocket_write(conn, WEBSOCKET_OPCODE_TEXT, message, strlen(message));
}

// Arguments:
//   flags: first byte of websocket frame, see websocket RFC,
//          http://tools.ietf.org/html/rfc6455, section 5.2
//   data, data_len: payload data. Mask, if any, is already applied.
static int websocket_data_handler(struct mg_connection *conn, int flags,
    char *data, size_t data_len)
{
  LOG_D ("Entering websocket_data_handler, connecting to batch server");
  /* start the connection */
  int sock_handle = connect_to_batch_server (server, port);

  LOG_D ("Processing incoming batch");
  StingerBatch * batch = new StingerBatch();
  batch->ParseFromString(std::string(data, data_len));

  batch->PrintDebugString();

  LOG_D ("Sending batch to batch server");
  send_message(sock_handle, *batch);
  close(sock_handle);


  LOG_D ("Reply to client");
  (void) flags; // Unused
  mg_websocket_write(conn, WEBSOCKET_OPCODE_TEXT, data, data_len);

  // Returning zero means stoping websocket conversation.
  // Close the conversation if client has sent us "exit" string.
  return memcmp(data, "exit", 4);
}

int main(int argc, char *argv[])
{
  struct mg_context *ctx;
  struct mg_callbacks callbacks;
  const char *options[] = {
    "listening_ports", "8080",
    "document_root", "data/websockets",
    NULL
  };

  int opt = 0;
  while(-1 != (opt = getopt(argc, argv, "p:a:"))) {
    switch(opt) {
      case 'p': {
	port = atoi(optarg);
      } break;

      case 'a': {
	server = gethostbyname(optarg);
	if(NULL == server) {
	  LOG_E_A ("ERROR: server %s could not be resolved.", optarg);
	  exit(-1);
	}
      } break;

      case '?':
      case 'h': {
	printf("Usage:    %s [-p port] [-a server_addr] [-n num_vertices] [-x batch_size] [-y num_batches]\n", argv[0]);
	printf("Defaults:\n\tport: %d\n\tserver: localhost\n", port);
	exit(0);
      } break;
    }
  }


  /* connect to localhost if server is unspecified */
  if(NULL == server) {
    server = gethostbyname("localhost");
    if(NULL == server) {
      LOG_E_A ("ERROR: server %s could not be resolved.", "localhost");
      exit(-1);
    }
  }

  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.websocket_ready = websocket_ready_handler;
  callbacks.websocket_data = websocket_data_handler;
  ctx = mg_start(&callbacks, NULL, options);
  getchar();  // Wait until user hits "enter"
  mg_stop(ctx);

  return 0;
}

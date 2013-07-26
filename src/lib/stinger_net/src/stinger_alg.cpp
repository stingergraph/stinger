#include "stinger_alg.h"

#include <netdb.h>
#include <cstdio>
#include <limits>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "proto/stinger-alg.pb.h"
#include "proto/stinger-connect.pb.h"
#include "send_rcv.h"

extern "C" {
  #include "stinger_core/xmalloc.h"
  #include "stinger_core/stinger_error.h"
  #include "stinger_core/stinger_shared.h"
}

using namespace gt::stinger;

extern "C" stinger_registered_alg *
stinger_register_alg_impl(stinger_register_alg_params params)
{
  LOG_D("Registering alg");

  struct sockaddr_in sock_addr;
  memset(&sock_addr, 0, sizeof(struct sockaddr_in));
  struct hostent * hp = gethostbyname(params.host);

  if(!hp) {
    LOG_E_A("Error resolving host %s", params.host);
    return NULL;
  }

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if(-1 == sock) {
    LOG_E("Error opening socket");
    return NULL;
  }

  memcpy(&sock_addr.sin_addr.s_addr, hp->h_addr, hp->h_length);
  sock_addr.sin_family = AF_INET;

  LOG_D_A("Socket open, connecting to host %s %s", params.host, inet_ntoa(**(struct in_addr **)hp->h_addr_list));

  if(params.port) {
    sock_addr.sin_port = htons(params.port);
  } else {
    sock_addr.sin_port = htons(10101);
  }

  if(-1 == connect(sock, (sockaddr *)&sock_addr, sizeof(struct sockaddr_in))) {
    LOG_E("Error connecting socket");
    return NULL;
  }

  LOG_D("Connected to server");

  AlgToServer alg_to_server;
  alg_to_server.set_alg_name(params.name);
  alg_to_server.set_action(REGISTER_ALG);

  if(params.data_per_vertex) {
    alg_to_server.set_data_per_vertex(params.data_per_vertex);
    if(!params.data_description) {
      LOG_E("Missing data description")
      return NULL;
    }
    alg_to_server.set_data_description(params.data_description);
  }

  for(int64_t i = 0; i < params.num_dependencies; i++) {
    alg_to_server.add_req_dep_name(params.dependencies[i]);
  }

  LOG_D("Sending message to server");
  Connect connect;
  connect.set_type(CLIENT_ALG);
  send_message(sock, connect);
  send_message(sock, alg_to_server);

  ServerToAlg server_to_alg;

  LOG_D("Waiting on message from server");
  recv_message(sock, server_to_alg);

  LOG_D("Received message from server");

  if(server_to_alg.result() != SUCCESS) {
    LOG_E("Error connecting to server");
    return NULL;
  }

  stinger_registered_alg * rtn = (stinger_registered_alg *) xmalloc(sizeof(stinger_registered_alg));

  if(!rtn) {
    LOG_E("Failed to allocate return");
    return NULL;
  }

  LOG_D_A("Mapping STINGER %s", server_to_alg.stinger_loc().c_str());
  rtn->sock = sock;
  rtn->stinger = stinger_shared_map(server_to_alg.stinger_loc().c_str(), server_to_alg.stinger_size());

  if(!rtn) {
    LOG_E("Failed to connect to stinger");
    free(rtn);
    return NULL;
  }
                                                
  strcpy(rtn->stinger_loc, server_to_alg.stinger_loc().c_str());
  LOG_D("STINGER mapped.");


  rtn->alg_data_per_vertex = params.data_per_vertex;
  if(params.data_per_vertex) {
    LOG_D_A("Mapping alg storage at %s", server_to_alg.alg_data_loc().c_str());
    strcpy(rtn->alg_data_loc, server_to_alg.alg_data_loc().c_str());
    rtn->alg_data = shmmap(server_to_alg.alg_data_loc().c_str(), O_RDWR, S_IRUSR | S_IWUSR, PROT_READ | PROT_WRITE, params.data_per_vertex * STINGER_MAX_LVERTICES, MAP_SHARED);
    if(!rtn->alg_data) {
      LOG_E("Mapping alg data failed");
    }
  } else {
    strcpy(rtn->alg_data_loc, "");
    rtn->alg_data = NULL;
  }

  LOG_D("Handling dependencies");

  rtn->batch = 0;
  rtn->dep_count = server_to_alg.dep_name_size();

  rtn->dep_name = (char **) xmalloc(sizeof(char *) * rtn->dep_count);
  rtn->dep_location = (char **) xmalloc(sizeof(char *) * rtn->dep_count);
  rtn->dep_data = (void **) xmalloc(sizeof(void *) * rtn->dep_count);
  rtn->dep_description= (char **) xmalloc(sizeof(char *) * rtn->dep_count);
  rtn->dep_data_per_vertex = (int64_t *) xmalloc(sizeof(int64_t) * rtn->dep_count);

  for(int64_t d = 0; d < server_to_alg.dep_name_size(); d++) {
    rtn->dep_name[d] = (char *)xmalloc(sizeof(char) * 256);
    rtn->dep_location[d] = (char *)xmalloc(sizeof(char) * 256);
    rtn->dep_description[d] = (char *)xmalloc(sizeof(char) * 256);
    
    rtn->dep_data[d] = shmmap(
      server_to_alg.dep_data_loc(d).c_str(), O_RDWR, S_IRUSR | S_IWUSR, PROT_READ | PROT_WRITE, 
      server_to_alg.dep_data_per_vertex(d) * STINGER_MAX_LVERTICES, MAP_SHARED);

    if(!rtn->dep_data[d]) {
      LOG_E_A("Failed to map data for %s, but continuing", server_to_alg.dep_name(d).c_str());
    }

    strcpy(rtn->dep_name[d], server_to_alg.dep_name(d).c_str());
    strcpy(rtn->dep_location[d], server_to_alg.dep_data_loc(d).c_str());
    strcpy(rtn->dep_description[d], server_to_alg.dep_description(d).c_str());
    rtn->dep_data_per_vertex[d] = server_to_alg.dep_data_per_vertex(d);
  }

  LOG_D_A("Algorithm %s successfully registered", params.name);

  return rtn;
}

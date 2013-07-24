#include "stinger_alg.h"

#include <netdb.h>
#include <cstdio>
#include <limits>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "proto/stinger-alg.pb.h"
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
  struct addrinfo * res;
  struct addrinfo hint;
  hint.ai_protocol = AF_INET;

  char portname[256];

  if(params.port) {
    sprintf(portname, "%d", params.port);
  } else {
    sprintf(portname, "10101");
  }

  if(getaddrinfo(params.host, portname, &hint, &res)) {
    LOG_E_A("Resolving %s %d failed", params.host, params.port);
    freeaddrinfo(res);
    return NULL;
  }

  int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if(-1 == sock) {
    LOG_E("Error opening socket");
    freeaddrinfo(res);
    return NULL;
  }

  if(-1 == connect(sock, res->ai_addr, res->ai_addrlen)) {
    LOG_E("Error connecting socket");
    freeaddrinfo(res);
    return NULL;
  }

  AlgToServer alg_to_server;
  alg_to_server.set_alg_name(params.name);
  alg_to_server.set_action(REGISTER_ALG);

  if(params.data_per_vertex) {
    alg_to_server.set_data_per_vertex(params.data_per_vertex);
    alg_to_server.set_data_description(params.data_description);
  }

  for(int64_t i = 0; i < params.num_dependencies; i++) {
    alg_to_server.add_req_dep_name(params.dependencies[i]);
  }

  send_message(sock, alg_to_server);

  ServerToAlg server_to_alg;

  recv_message(sock, server_to_alg);

  if(server_to_alg.result() != SUCCESS) {
    LOG_E("Error connecting socket");
    freeaddrinfo(res);
    return NULL;
  }

  stinger_registered_alg * rtn = (stinger_registered_alg *) xmalloc(sizeof(stinger_registered_alg));

  if(!rtn) {
    LOG_E("Failed to allocate return");
    freeaddrinfo(res);
    return NULL;
  }

  rtn->sock = sock;
  rtn->stinger = stinger_shared_map(server_to_alg.stinger_loc().c_str(), server_to_alg.stinger_size());

  if(!rtn) {
    LOG_E("Failed to connect to stinger");
    freeaddrinfo(res);
    free(rtn);
    return NULL;
  }

  strcpy(rtn->stinger_loc, server_to_alg.stinger_loc().c_str());
  strcpy(rtn->alg_data_loc, server_to_alg.alg_data_loc().c_str());
  rtn->alg_data = shmmap(server_to_alg.alg_data_loc().c_str(), O_RDWR, S_IRUSR | S_IWUSR, PROT_READ | PROT_WRITE, params.data_per_vertex * STINGER_MAX_LVERTICES, MAP_SHARED);
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

  freeaddrinfo(res);
  return rtn;
}

#include <netdb.h>
#include <cstdio>
#include <limits>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "proto/stinger-alg.pb.h"
#include "proto/stinger-connect.pb.h"
#include "send_rcv.h"

//#define LOG_AT_W
#include "stinger_core/stinger_error.h"
#include "stinger_core/stinger_shared.h"
#include "stinger_core/xmalloc.h"
#include "stinger_alg.h"


using namespace gt::stinger;

extern "C" stinger_registered_alg *
stinger_register_alg_impl(stinger_register_alg_params params)
{
  LOG_D_A("Registering alg %s", params.name);

  int sock = connect_to_server(params.host, params.port ? params.port : 10103);
  if (sock == -1) { return NULL; }

  LOG_D("Connected to server");

  AlgToServer alg_to_server;
  alg_to_server.set_alg_name(params.name);
  alg_to_server.set_action(REGISTER_ALG);

  LOG_D("Alg message built");

  if(params.data_per_vertex) {
    LOG_D("Setting data per vertex");
    alg_to_server.set_data_per_vertex(params.data_per_vertex);
    if(!params.data_description) {
      LOG_E("Missing data description")
      return NULL;
    }
    alg_to_server.set_data_description(params.data_description);
  }

  LOG_D("About to set dependencies");
  LOG_D_A("Count is [%ld]", params.num_dependencies);
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

  if(server_to_alg.result() != ALG_SUCCESS) {
    LOG_E("Error connecting to server");
    return NULL;
  }

  stinger_registered_alg * rtn = (stinger_registered_alg *) xcalloc(1, sizeof(stinger_registered_alg));

  if(!rtn) {
    LOG_E("Failed to allocate return");
    return NULL;
  }

  strncpy(rtn->alg_name, server_to_alg.alg_name().c_str(),255);
  rtn->alg_num = server_to_alg.alg_num();

  if(!params.is_remote) {
    LOG_D_A("Mapping STINGER %s", server_to_alg.stinger_loc().c_str());
    rtn->sock = sock;
    if(params.map_private) {
      rtn->stinger = stinger_shared_private(server_to_alg.stinger_loc().c_str(), server_to_alg.stinger_size());
      rtn->map_private = 1;
    } else {
      rtn->stinger = stinger_shared_map(server_to_alg.stinger_loc().c_str(), server_to_alg.stinger_size());
      rtn->map_private = 0;
    }

    if(!rtn) {
      LOG_E("Failed to connect to stinger");
      free(rtn);
      return NULL;
    }
						  
    strcpy(rtn->stinger_loc, server_to_alg.stinger_loc().c_str());
    LOG_D("STINGER mapped.");
  }


  rtn->alg_data_per_vertex = params.data_per_vertex;
  if(params.data_per_vertex) {
    LOG_D_A("Mapping alg storage at %s", server_to_alg.alg_data_loc().c_str());
    strcpy(rtn->alg_data_loc, server_to_alg.alg_data_loc().c_str());
    rtn->alg_data = shmmap(server_to_alg.alg_data_loc().c_str(), O_RDWR, S_IRUSR | S_IWUSR, PROT_READ | PROT_WRITE, params.data_per_vertex * rtn->stinger->max_nv, MAP_SHARED);
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
      server_to_alg.dep_data_per_vertex(d) * rtn->stinger->max_nv, MAP_SHARED);

    if(!rtn->dep_data[d]) {
      LOG_E_A("Failed to map data for %s, but continuing", server_to_alg.dep_name(d).c_str());
    }

    strcpy(rtn->dep_name[d], server_to_alg.dep_name(d).c_str());
    strcpy(rtn->dep_location[d], server_to_alg.dep_data_loc(d).c_str());
    strcpy(rtn->dep_description[d], server_to_alg.dep_description(d).c_str());
    rtn->dep_data_per_vertex[d] = server_to_alg.dep_data_per_vertex(d);
  }

  rtn->enabled = true;
  LOG_D_A("Algorithm %s successfully registered", params.name);

  return rtn;
}

extern "C" stinger_registered_alg *
stinger_alg_begin_init(stinger_registered_alg * alg)
{
  LOG_D("Requesting init");

  AlgToServer alg_to_server;
  alg_to_server.set_alg_name(alg->alg_name);
  alg_to_server.set_alg_num(alg->alg_num);
  alg_to_server.set_action(BEGIN_INIT);

  LOG_D("Sending message to server");
  send_message(alg->sock, alg_to_server);

  ServerToAlg server_to_alg;

  LOG_D("Waiting on message from server");
  recv_message(alg->sock, server_to_alg);

  LOG_D("Received message from server");

  if(server_to_alg.result() != ALG_SUCCESS || server_to_alg.action() != alg_to_server.action()) {
    LOG_E("Error - failure at server or incorrect action returned");
    alg->enabled = false;
    return NULL;
  }

  LOG_D_A("Algorithm %s ready for init", alg->alg_name);

  return alg;
}

extern "C" stinger_registered_alg *
stinger_alg_end_init(stinger_registered_alg * alg)
{
  LOG_D("Ending init");

  AlgToServer alg_to_server;
  alg_to_server.set_alg_name(alg->alg_name);
  alg_to_server.set_alg_num(alg->alg_num);
  alg_to_server.set_action(END_INIT);

  LOG_D("Sending message to server");
  send_message(alg->sock, alg_to_server);

  ServerToAlg server_to_alg;

  LOG_D("Waiting on message from server");
  recv_message(alg->sock, server_to_alg);

  LOG_D("Received message from server");

  if(server_to_alg.result() != ALG_SUCCESS || server_to_alg.action() != alg_to_server.action()) {
    LOG_E("Error - failure at server or incorrect action returned");
    alg->enabled = false;
    return NULL;
  }

  LOG_D_A("Algorithm %s has completed init", alg->alg_name);

  return alg;
}

extern "C" stinger_registered_alg *
stinger_alg_begin_pre(stinger_registered_alg * alg)
{
  LOG_D("Requesting pre");

  AlgToServer alg_to_server;
  alg_to_server.set_alg_name(alg->alg_name);
  alg_to_server.set_alg_num(alg->alg_num);
  alg_to_server.set_action(BEGIN_PREPROCESS);

  LOG_D("Sending message to server");
  send_message(alg->sock, alg_to_server);

  ServerToAlg * server_to_alg = NULL;
  if(!alg->batch_storage) {
    server_to_alg = new ServerToAlg();
    alg->batch_storage = (void *)server_to_alg;
  } else {
    server_to_alg = (ServerToAlg *)alg->batch_storage;
  }

  LOG_D("Waiting on message from server");
  recv_message(alg->sock, *server_to_alg);

  LOG_D("Received message from server");

  if(server_to_alg->result() != ALG_SUCCESS || server_to_alg->action() != alg_to_server.action()) {
    LOG_E("Error - failure at server or incorrect action returned");
    alg->enabled = false;
    return NULL;
  }

  alg->num_insertions = server_to_alg->batch().insertions_size();

  if(alg->insertions) {
    free(alg->insertions);
  }

  alg->insertions = (stinger_edge_update *)xcalloc(alg->num_insertions, sizeof(stinger_edge_update));

  alg->num_deletions  = server_to_alg->batch().deletions_size();

  if(alg->deletions) {
    free(alg->deletions);
  }

  alg->deletions = (stinger_edge_update *)xcalloc(alg->num_deletions, sizeof(stinger_edge_update));

  alg->num_vertex_updates  = server_to_alg->batch().vertex_updates_size();

  if(alg->vertex_updates) {
    free(alg->vertex_updates);
  }

  alg->vertex_updates = (stinger_vertex_update *)xcalloc(alg->num_vertex_updates, sizeof(stinger_vertex_update));

  alg->num_metadata  = server_to_alg->batch().metadata_size();

  if(alg->metadata) {
    free(alg->metadata);
  }

  alg->metadata = (uint8_t **)xcalloc(alg->num_metadata, sizeof(uint8_t *));

  if(alg->metadata_lengths) {
    free(alg->metadata_lengths);
  }

  alg->metadata_lengths = (uint64_t *)xcalloc(alg->num_metadata, sizeof(uint64_t));

  switch(server_to_alg->batch().type()) {
    case NUMBERS_ONLY: {
      alg->batch_type = BATCH_NUMBERS_ONLY;
      OMP("omp parallel for")
	for (size_t i = 0; i < server_to_alg->batch().insertions_size(); i++) {
	  const EdgeInsertion & in = server_to_alg->batch().insertions(i);
	  alg->insertions[i].type	  = in.type();
	  alg->insertions[i].source	  = in.source();
	  alg->insertions[i].destination  = in.destination();
	  alg->insertions[i].weight	  = in.weight();
	  alg->insertions[i].time	  = in.time();
	  alg->insertions[i].result	  = in.result();
	  alg->insertions[i].meta_index	  = in.meta_index();
	}

      OMP("omp parallel for")
	for(size_t d = 0; d < server_to_alg->batch().deletions_size(); d++) {
	  const EdgeDeletion & del	= server_to_alg->batch().deletions(d);
	  alg->deletions[d].type	= del.type();
	  alg->deletions[d].source	= del.source();
	  alg->deletions[d].destination	= del.destination();
	  alg->deletions[d].result	= del.result();
	  alg->deletions[d].meta_index	= del.meta_index();
	}
    } break;

    case STRINGS_ONLY: {
      alg->batch_type = BATCH_STRINGS_ONLY;
      OMP("omp parallel for")
	for (size_t i = 0; i < server_to_alg->batch().insertions_size(); i++) {
	  const EdgeInsertion & in = server_to_alg->batch().insertions(i);
	  alg->insertions[i].type_str	      = in.type_str().c_str();
	  alg->insertions[i].source_str	      = in.source_str().c_str();
	  alg->insertions[i].destination_str  = in.destination_str().c_str();
	  alg->insertions[i].weight	      = in.weight();
	  alg->insertions[i].time	      = in.time();
	  alg->insertions[i].result	      = in.result();
	  alg->insertions[i].meta_index	      = in.meta_index();
	}

      OMP("omp parallel for")
	for(size_t d = 0; d < server_to_alg->batch().deletions_size(); d++) {
	  const EdgeDeletion & del = server_to_alg->batch().deletions(d);
	  alg->deletions[d].type_str	      = del.type_str().c_str();
	  alg->deletions[d].source_str	      = del.source_str().c_str();
	  alg->deletions[d].destination_str   = del.destination_str().c_str();
	  alg->deletions[d].result	      = del.result();
	  alg->deletions[d].meta_index	      = del.meta_index();
	}
    } break;

    case MIXED: {
      alg->batch_type = BATCH_MIXED;
      OMP("omp parallel for")
	for (size_t i = 0; i < server_to_alg->batch().insertions_size(); i++) {
	  const EdgeInsertion & in = server_to_alg->batch().insertions(i);
          if(in.has_type_str()) {
	    alg->insertions[i].type_str		= in.type_str().c_str();
	  } else {
	    alg->insertions[i].type		= in.type();
	  }

	  if(in.has_source_str()) {
	    alg->insertions[i].source_str	= in.source_str().c_str();
	  } else {
	    alg->insertions[i].source		= in.source();
	  }

	  if(in.has_destination_str()) {
	    alg->insertions[i].destination_str	= in.destination_str().c_str();
	  } else {
	    alg->insertions[i].destination      = in.destination();
	  }

	  alg->insertions[i].weight	      = in.weight();
	  alg->insertions[i].time	      = in.time();
	  alg->insertions[i].result	      = in.result();
	  alg->insertions[i].meta_index	      = in.meta_index();
	}

      OMP("omp parallel for")
	for(size_t d = 0; d < server_to_alg->batch().deletions_size(); d++) {
	  const EdgeDeletion & del = server_to_alg->batch().deletions(d);
          if(del.has_type_str()) {
	    alg->deletions[d].type_str		= del.type_str().c_str();
	  } else {
	    alg->deletions[d].type		= del.type();
	  }

	  if(del.has_source_str()) {
	    alg->deletions[d].source_str	= del.source_str().c_str();
	  } else {
	    alg->deletions[d].source		= del.source();
	  }

	  if(del.has_destination_str()) {
	    alg->deletions[d].destination_str	= del.destination_str().c_str();
	  } else {
	    alg->deletions[d].destination      = del.destination();
	  }

	  alg->deletions[d].result	      = del.result();
	  alg->deletions[d].meta_index	      = del.meta_index();
	}
    } break;
  }

  OMP("omp parallel for")
  for(size_t v = 0; v < server_to_alg->batch().vertex_updates_size(); v++) {
    const VertexUpdate & up = server_to_alg->batch().vertex_updates(v);
    if(up.has_type_str()) {
      alg->vertex_updates[v].type_str		= up.type_str().c_str();
    } else {
      alg->vertex_updates[v].type		= up.type();
    }

    if(up.has_vertex_str()) {
      alg->vertex_updates[v].vertex_str	        = up.vertex_str().c_str();
    } else {
      alg->vertex_updates[v].vertex		= up.vertex();
    }

    alg->vertex_updates[v].set_weight           = up.set_weight();
    alg->vertex_updates[v].incr_weight          = up.incr_weight();
    alg->vertex_updates[v].meta_index	        = up.meta_index();
  }

  OMP("omp parallel for")
  for(size_t m = 0; m < server_to_alg->batch().metadata_size(); m++) {
    const std::string & meta = server_to_alg->batch().metadata(m);
    alg->metadata[m]         = (uint8_t *)meta.c_str();
    alg->metadata_lengths[m] = meta.size();
  }

  /* added in d9c4ac2 -- algorithms cannot write to the data structure */
  /* map_names (alg); */

  LOG_D_A("Algorithm %s ready for pre", alg->alg_name);

  return alg;
}

/* added in d9c4ac2 -- algorithms cannot write to the data structure */
/* This functionality needs to be moved to the server if necessary */
//void
//map_names (stinger_registered_alg * alg)
//{
//  stinger_t * S = alg->stinger;
//  int64_t src_id, dst_id, type_id;
//
//  OMP("omp for")
//    for (size_t i = 0; i < alg->num_insertions; i++) {
//      // if (alg->insertions[i].type_str) {
//      //   int64_t t;
//      //   fprintf (stderr, "sigh %s\n", alg->insertions[i].type_str);
//      //   stinger_etype_names_create_type (S, alg->insertions[i].type_str, &t);
//      //   alg->insertions[i].type = t;
//      // }
//      if (alg->insertions[i].source_str) {
//        int64_t t;
//        stinger_mapping_create (S, alg->insertions[i].source_str,
//                                strlen (alg->insertions[i].source_str), &t);
//        alg->insertions[i].source = t;
//      }
//      if (alg->insertions[i].destination_str) {
//        int64_t t;
//        stinger_mapping_create (S, alg->insertions[i].destination_str,
//                                strlen (alg->insertions[i].destination_str), &t);
//        alg->insertions[i].destination = t;
//      }
//    }
//
//  OMP("omp for")
//    for(size_t d = 0; d < alg->num_deletions; d++) {
//      // if (alg->deletions[d].type_str) {
//      //   int64_t t;
//      //   stinger_etype_names_create_type (S, alg->deletions[d].type_str, &t);
//      //   alg->deletions[d].type = t;
//      // }
//      if (alg->deletions[d].source_str) {
//        int64_t t;
//        stinger_mapping_create (S, alg->deletions[d].source_str,
//                                strlen (alg->deletions[d].source_str), &t);
//        alg->deletions[d].source = t;
//      }
//      if (alg->deletions[d].destination_str) {
//        int64_t t;
//        stinger_mapping_create (S, alg->deletions[d].destination_str,
//                                strlen (alg->deletions[d].destination_str), &t);
//        alg->deletions[d].destination = t;
//      }
//    }
//
//  // OMP("omp for")
//  //   for(size_t v = 0; v < alg->num_vertex_updates; v++) {
//  //     if (alg->vertex_updates[v].type_str) {
//  //       int64_t t;
//  //       stinger_vtype_names_create_type (S, alg->vertex_updates[v].type_str, &t);
//  //       alg->vertex_updates[v].type = t;
//  //     }
//
//  //     if (alg->vertex_updates[v].vertex_str) {
//  //       int64_t t;
//  //       stinger_mapping_create (S, alg->vertex_updates[v].vertex_str, strlen (alg->vertex_updates[v].vertex_str), &t);
//  //       alg->vertex_updates[v].vertex = t;
//  //     }
//  //   }
//}

extern "C" stinger_registered_alg *
stinger_alg_end_pre(stinger_registered_alg * alg)
{
  LOG_D("Ending pre");

  AlgToServer alg_to_server;
  alg_to_server.set_alg_name(alg->alg_name);
  alg_to_server.set_alg_num(alg->alg_num);
  alg_to_server.set_action(END_PREPROCESS);

  LOG_D("Sending message to server");
  send_message(alg->sock, alg_to_server);

  ServerToAlg * server_to_alg = NULL;
  if(!alg->batch_storage) {
    server_to_alg = new ServerToAlg();
    alg->batch_storage = (void *)server_to_alg;
  } else {
    server_to_alg = (ServerToAlg *)alg->batch_storage;
  }

  LOG_D("Waiting on message from server");
  recv_message(alg->sock, *server_to_alg);

  LOG_D("Received message from server");

  if(server_to_alg->result() != ALG_SUCCESS || server_to_alg->action() != alg_to_server.action()) {
    LOG_E("Error - failure at server or incorrect action returned");
    alg->enabled = false;
    return NULL;
  }

  LOG_D_A("Algorithm %s has completed pre", alg->alg_name);

  return alg;
}

extern "C" stinger_registered_alg *
stinger_alg_begin_post(stinger_registered_alg * alg)
{
  LOG_D("Requesting post");

  AlgToServer alg_to_server;
  alg_to_server.set_alg_name(alg->alg_name);
  alg_to_server.set_alg_num(alg->alg_num);
  alg_to_server.set_action(BEGIN_POSTPROCESS);

  LOG_D("Sending message to server");
  send_message(alg->sock, alg_to_server);

  ServerToAlg * server_to_alg = NULL;
  if(!alg->batch_storage) {
    server_to_alg = new ServerToAlg();
    alg->batch_storage = (void *)server_to_alg;
  } else {
    server_to_alg = (ServerToAlg *)alg->batch_storage;
  }

  LOG_D("Waiting on message from server");
  recv_message(alg->sock, *server_to_alg);

  LOG_D("Received message from server");

  if(server_to_alg->result() != ALG_SUCCESS || server_to_alg->action() != alg_to_server.action()) {
    LOG_E("Error - failure at server or incorrect action returned");
    alg->enabled = false;
    return NULL;
  }

  switch(server_to_alg->batch().type()) {
    case NUMBERS_ONLY: {
      OMP("omp parallel for")
	for (size_t i = 0; i < server_to_alg->batch().insertions_size(); i++) {
	  const EdgeInsertion & in	      = server_to_alg->batch().insertions(i);
	  alg->insertions[i].type_str	      = in.type_str().c_str();
	  alg->insertions[i].source_str	      = in.source_str().c_str();
	  alg->insertions[i].destination_str  = in.destination_str().c_str();
	  alg->insertions[i].result	      = in.result();
	}

      OMP("omp parallel for")
	for(size_t d = 0; d < server_to_alg->batch().deletions_size(); d++) {
	  const EdgeDeletion & del	      = server_to_alg->batch().deletions(d);
	  alg->deletions[d].type_str	      = del.type_str().c_str();
	  alg->deletions[d].source_str	      = del.source_str().c_str();
	  alg->deletions[d].destination_str   = del.destination_str().c_str();
	  alg->deletions[d].result	      = del.result();
	}
    } break;

    case STRINGS_ONLY: {
      OMP("omp parallel for")
	for (size_t i = 0; i < server_to_alg->batch().insertions_size(); i++) {
	  const EdgeInsertion & in	      = server_to_alg->batch().insertions(i);
	  alg->insertions[i].type	      = in.type();
	  alg->insertions[i].source	      = in.source();
	  alg->insertions[i].destination      = in.destination();
	  alg->insertions[i].type_str	      = in.type_str().c_str();
	  alg->insertions[i].source_str	      = in.source_str().c_str();
	  alg->insertions[i].destination_str  = in.destination_str().c_str();
	  alg->insertions[i].result	      = in.result();
	}

      OMP("omp parallel for")
	for(size_t d = 0; d < server_to_alg->batch().deletions_size(); d++) {
	  const EdgeDeletion & del	      = server_to_alg->batch().deletions(d);
	  alg->deletions[d].type	      = del.type();
	  alg->deletions[d].source	      = del.source();
	  alg->deletions[d].destination	      = del.destination();
	  alg->deletions[d].type_str	      = del.type_str().c_str();
	  alg->deletions[d].source_str	      = del.source_str().c_str();
	  alg->deletions[d].destination_str   = del.destination_str().c_str();
	  alg->deletions[d].result	      = del.result();
	}
    } break;

    case MIXED: {
      OMP("omp parallel for")
	for (size_t i = 0; i < server_to_alg->batch().insertions_size(); i++) {
	  const EdgeInsertion & in		= server_to_alg->batch().insertions(i);
	  alg->insertions[i].type_str		= in.type_str().c_str();
	  alg->insertions[i].type		= in.type();

	  alg->insertions[i].source_str		= in.source_str().c_str();
	  alg->insertions[i].source		= in.source();

	  alg->insertions[i].destination_str	= in.destination_str().c_str();
	  alg->insertions[i].destination	= in.destination();
	  alg->insertions[i].result	        = in.result();
	}

      OMP("omp parallel for")
	for(size_t d = 0; d < server_to_alg->batch().deletions_size(); d++) {
	  const EdgeDeletion & del		= server_to_alg->batch().deletions(d);
	  alg->deletions[d].type_str		= del.type_str().c_str();
	  alg->deletions[d].type		= del.type();

	  alg->deletions[d].source_str		= del.source_str().c_str();
	  alg->deletions[d].source		= del.source();

	  alg->deletions[d].destination_str	= del.destination_str().c_str();
	  alg->deletions[d].destination	        = del.destination();
	  alg->deletions[d].result	        = del.result();
	}
    } break;
  }

  OMP("omp parallel for")
  for(size_t v = 0; v < server_to_alg->batch().vertex_updates_size(); v++) {
    const VertexUpdate & up = server_to_alg->batch().vertex_updates(v);
    alg->vertex_updates[v].type_str		= up.type_str().c_str();
    alg->vertex_updates[v].type		= up.type();

    alg->vertex_updates[v].vertex_str	        = up.vertex_str().c_str();
    alg->vertex_updates[v].vertex		= up.vertex();

    alg->vertex_updates[v].set_weight           = up.set_weight();
    alg->vertex_updates[v].incr_weight          = up.incr_weight();
    alg->vertex_updates[v].meta_index	        = up.meta_index();
  }

  /* added in d9c4ac2 -- algorithms cannot write to the data structure */
  /* map_names (alg); */

  LOG_D_A("Algorithm %s ready for post", alg->alg_name);

  return alg;
}

extern "C" stinger_registered_alg *
stinger_alg_end_post(stinger_registered_alg * alg)
{
  LOG_D("Ending post");

  AlgToServer alg_to_server;
  alg_to_server.set_alg_name(alg->alg_name);
  alg_to_server.set_alg_num(alg->alg_num);
  alg_to_server.set_action(END_POSTPROCESS);

  LOG_D("Sending message to server");
  send_message(alg->sock, alg_to_server);
 
  ServerToAlg * server_to_alg = NULL;
  if(!alg->batch_storage) {
    server_to_alg = new ServerToAlg();
    alg->batch_storage = (void *)server_to_alg;
  } else {
    server_to_alg = (ServerToAlg *)alg->batch_storage;
  }

  LOG_D("Waiting on message from server");
  recv_message(alg->sock, *server_to_alg);

  LOG_D("Received message from server");

  if(server_to_alg->result() != ALG_SUCCESS || server_to_alg->action() != alg_to_server.action()) {
    LOG_E("Error - failure at server or incorrect action returned");
    alg->enabled = false;
    return NULL;
  }

  LOG_D_A("Algorithm %s has completed post", alg->alg_name);

  return alg;
}

extern "C" void
stinger_alg_weight_double(stinger_t * S, double * field, double weight) {
  int64_t nv = stinger_mapping_nv(S);

  OMP("omp parallel for")
  for(int64_t v = 0; v < nv; v++) {
    field[v] *= weight;
  }
}

extern "C" void
stinger_alg_weight_int64(stinger_t * S, int64_t * field, int64_t weight) {
  int64_t nv = stinger_mapping_nv(S);

  OMP("omp parallel for")
  for(int64_t v = 0; v < nv; v++) {
    field[v] *= weight;
  }
}

extern "C" int64_t
stinger_alg_max_vertices(stinger_t * S) {
  return S->max_nv;
}

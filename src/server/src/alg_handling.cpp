#include "stinger_core/stinger.h"
#include "stinger_core/formatting.h"
#include "stinger_core/stinger_shared.h"
#include "stinger_utils/timer.h"
#include "stinger_utils/stinger_utils.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/x86_full_empty.h"
#include "stinger_net/proto/stinger-batch.pb.h"
#include "stinger_net/proto/stinger-connect.pb.h"
#include "stinger_net/proto/stinger-alg.pb.h"
#include "stinger_net/send_rcv.h"
#include "stinger_net/stinger_server_state.h"
#include "server.h"

#include <cstdio>
#include <limits>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>

/* POSIX only for now, note that mongoose would be a good place to 
get cross-platform threading and sockets code */
#include <pthread.h> 

/* Must be included last */
//#define LOG_AT_I
#include "stinger_core/stinger_error.h"

/* A note on the design of this server:
 * Main thread -> starts all other threads, listens to the incoming port and accepts connections.
 * Stream threads -> One per stream / alg, receives a batch from a stream and enqueues the result. For algorithms, perform the initial setup, then main loop handles
 * Process loop thread -> handles all request
 */

/* TODO -> perhaps the timeout for a given step should be split among the levels such that 
  each level is allowed to have until the deadline for the window it would have assuming the other levels use
  all of theirs.  Note that currently, the amount of time that a stage can take is really levels * window size */

using namespace gt::stinger;

struct AcceptedSock {
  socklen_t len;
  struct sockaddr addr;
  int handle;
};

void *
test_thread_handler(void *) {
  StingerServerState & server_state = StingerServerState::get_server_state();
  while(1) {
    usleep(500000);
    StingerBatch * batch = new StingerBatch();
    server_state.enqueue_batch(batch);
  }
  return NULL;
}

void
handle_alg(struct AcceptedSock * sock, StingerServerState & server_state)
{
  AlgToServer alg_to_server;
  if(recv_message(sock->handle, alg_to_server)) {

    LOG_D_A("Received new algorithm.  Printing incoming protobuf: %s", alg_to_server.DebugString().c_str());

    ServerToAlg server_to_alg;
    server_to_alg.set_alg_name(alg_to_server.alg_name());
    server_to_alg.set_action(alg_to_server.action());
    server_to_alg.set_stinger_loc(server_state.get_stinger_loc());
    server_to_alg.set_stinger_size(sizeof(stinger_t) + server_state.get_stinger()->length);
    server_to_alg.set_result(ALG_SUCCESS);

    if(alg_to_server.action() != REGISTER_ALG) {
      LOG_E("Algorithm failed to register");
      server_to_alg.set_result(ALG_FAILURE_GENERIC);
      send_message(sock->handle, server_to_alg);
      return;
    }

    if(server_state.has_alg(alg_to_server.alg_name())) {
      if(server_state.get_alg(alg_to_server.alg_name())->state < ALG_STATE_DONE) {
        LOG_E("Algorithm already exists");
        server_to_alg.set_result(ALG_FAILURE_NAME_EXISTS);
        send_message(sock->handle, server_to_alg);
        return;
      } else {
        server_state.delete_alg(alg_to_server.alg_name());
      }
    }

    /* check name and attempt to allocate space */
    if(is_simple_name(alg_to_server.alg_name().c_str(), alg_to_server.alg_name().length())) {
      int64_t data_total = alg_to_server.data_per_vertex() * server_state.get_stinger()->max_nv;
      void * data = NULL;
      char map_name[256] = "";

      if(data_total) {
        sprintf(map_name, "/%s", alg_to_server.alg_name().c_str());
        LOG_D_A("Attempting to map %" PRId64 " at %s", data_total, map_name);
        data = shmmap(map_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, PROT_READ | PROT_WRITE, data_total, MAP_SHARED);

        if(!data) {
          LOG_E_A("Error, mapping storage for algorithm %s failed (%" PRId64 " bytes per vertex)", 
            alg_to_server.alg_name().c_str(), alg_to_server.data_per_vertex());
          server_to_alg.set_result(ALG_FAILURE_GENERIC);
          send_message(sock->handle, server_to_alg);
          return;
        }
        server_to_alg.set_alg_data_loc(map_name);
        LOG_D("Map success")
      }

      LOG_D("Creating algorithm structure")

      StingerAlgState * alg_state = new StingerAlgState();
      alg_state->name = alg_to_server.alg_name();
      alg_state->data_loc = map_name;
      alg_state->data = data;
      alg_state->data_per_vertex = alg_to_server.data_per_vertex();
      alg_state->data_description = alg_to_server.data_description();
      alg_state->sock_handle = sock->handle;

      LOG_D("Resolving dependencies")

      int64_t max_level = 0;
      /*bool deps_resolved = true;*/
      for(int64_t i = 0; i < alg_to_server.req_dep_name_size(); i++) {
        const std::string & req_dep_name = alg_to_server.req_dep_name(i);
        if(server_state.has_alg(req_dep_name)) {
          StingerAlgState * dep_state = server_state.get_alg(req_dep_name);
          int64_t dep_level = dep_state->level;
          if(dep_level >= max_level) {
            max_level = dep_level + 1;
          }
          server_to_alg.add_dep_name(dep_state->name);
          server_to_alg.add_dep_description(dep_state->data_description);
          server_to_alg.add_dep_data_per_vertex(dep_state->data_per_vertex);
          server_to_alg.add_dep_data_loc(dep_state->data_loc);
        } else {
          LOG_E_A("Algorithm <%s> requested non-existant dependency <%s>.", 
            alg_to_server.alg_name().c_str(), req_dep_name.c_str());

          ServerToAlg server_to_alg;
          server_to_alg.set_alg_name(alg_to_server.alg_name());
          server_to_alg.set_action(alg_to_server.action());
          server_to_alg.set_result(ALG_FAILURE_DEPENDENCY);

          /*deps_resolved = false;*/
                shmunmap(map_name, data, data_total);
                shmunlink(map_name);

          delete alg_state;
          return;
        }
      }

      alg_state->level = max_level;

      LOG_V_A("Adding algorithm %s at level %" PRId64, alg_to_server.alg_name().c_str(), max_level);

      server_to_alg.set_alg_num(server_state.add_alg(max_level, alg_state));

      LOG_V("Algorithm added. Sending response");

      send_message(sock->handle, server_to_alg);
    } else {
      LOG_E_A("Error, requested name is not valid: %s",
      alg_to_server.alg_name().c_str());

      ServerToAlg server_to_alg;
      server_to_alg.set_alg_name(alg_to_server.alg_name());
      server_to_alg.set_action(alg_to_server.action());
      server_to_alg.set_result(ALG_FAILURE_NAME_INVALID);
      return;
    }
  }
}

void
handle_mon(struct AcceptedSock * sock, StingerServerState & server_state)
{
  MonToServer mon_to_server;
  if(recv_message(sock->handle, mon_to_server)) {

    LOG_D_A("Received new algorithm.  Printing incoming protobuf: %s", mon_to_server.DebugString().c_str());

    ServerToMon * server_to_mon = server_state.get_server_to_mon_copy();
    server_to_mon->set_mon_name(mon_to_server.mon_name());
    server_to_mon->set_action(mon_to_server.action());
    server_to_mon->set_result(MON_SUCCESS);

    if(mon_to_server.action() != REGISTER_MON) {
      LOG_E("Monitor failed to register");
      server_to_mon->set_result(MON_FAILURE_GENERIC);
      send_message(sock->handle, *server_to_mon);
      return;
    }

/* Commenting these lines out so that monitors can reconnect easily.
 * Someone could suggest a more permanent solution.
 * DE 11/13/2014
 */
    if(server_state.has_mon(mon_to_server.mon_name())) {
//      if(server_state.get_mon(mon_to_server.mon_name())->state < MON_STATE_DONE) {
//	LOG_E("Monitor already exists");
//	server_to_mon->set_result(MON_FAILURE_NAME_EXISTS);
//	send_message(sock->handle, *server_to_mon);
//	return;
 //     } else {
	server_state.delete_mon(mon_to_server.mon_name());
//      }
    }

    LOG_D("Creating monitor structure")

    StingerMonState * mon_state = new StingerMonState();
    mon_state->name = mon_to_server.mon_name();
    mon_state->sock_handle = sock->handle;

    LOG_V_A("Adding monitor %s", mon_to_server.mon_name().c_str());
    server_to_mon->set_mon_num(server_state.add_mon(mon_state));
    
    LOG_V_A("Monitor added. Sending response:\n\t%s", server_to_mon->DebugString().c_str());

    send_message(sock->handle, *server_to_mon);

    delete server_to_mon;
  }
}

int
can_be_read(int fd) {
  fd_set rfds;
  struct timeval tv;
  memset(&tv, 0, sizeof(tv));
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);

  int retval = select(fd+1, &rfds, NULL, NULL, &tv);

  if (retval == -1) {
    LOG_E("Socket error in select");
    return 0;
  } else if (retval) {
    return 1;
  } else {
    return 0;
  }
}

void *
new_connection_handler(void * data)
{
  StingerServerState & server_state = StingerServerState::get_server_state();
  struct AcceptedSock * accepted_sock = (struct AcceptedSock *)data;

  Connect connect_type;
  recv_message(accepted_sock->handle, connect_type);

  switch(connect_type.type()) {
    case CLIENT_STREAM:
      LOG_W("Client streams are unhandled");
    break;

    case CLIENT_ALG:
      LOG_V("Received algorithm client connection");
      handle_alg(accepted_sock, server_state);
    break;

    case CLIENT_MONITOR:
      LOG_V("Received monitor client connection");
      handle_mon(accepted_sock, server_state);
    break;

    default:
      LOG_E("Received unknown connection type");
    break;
  }

  free(accepted_sock);
  return NULL;
}

void *
process_loop_handler(void * data)
{
  LOG_V("Main loop thread started");
  double batch_time;
  double update_time;

  StingerServerState & server_state = StingerServerState::get_server_state();
  struct stinger * S = server_state.get_stinger();

  while(1) { /* TODO clean shutdown mechanism */
    StingerBatch * batch = server_state.dequeue_batch();

    batch_time = timer();
    /* loop through each algorithm, blocking init as needed, send start preprocessing message */
    size_t stop_alg_level = server_state.get_num_levels();
    for(size_t cur_level_index = 0; cur_level_index < stop_alg_level; cur_level_index++) {
      size_t stop_alg_index = server_state.get_num_algs(cur_level_index);
      for(size_t cur_alg_index = 0; cur_alg_index < stop_alg_index; cur_alg_index++) {
        StingerAlgState * cur_alg = server_state.get_alg(cur_level_index, cur_alg_index);

        /* for algorithms in the error state, eat the next message and assume that 
         * it will go back to trying to init */
        if(cur_alg->state == ALG_STATE_ERROR && can_be_read(cur_alg->sock_handle)) {
          AlgToServer alg_to_server;

          if(recv_message(cur_alg->sock_handle, alg_to_server) &&
            alg_to_server.alg_name().compare(cur_alg->name) == 0) {
            
              cur_alg->state = ALG_STATE_READY_INIT;
          }
        }

        if(cur_alg->state == ALG_STATE_READY_INIT) {
          AlgToServer alg_to_server;

          int64_t timeout = 0;
          while(!can_be_read(cur_alg->sock_handle) && timeout < server_state.alg_timeout(ALG_STATE_READY_INIT)) {
            usleep(server_state.time_granularity());
            //timeout += server_state.time_granularity();
          }

          if(!can_be_read(cur_alg->sock_handle)) {
            LOG_E_A("Algorithm <%s> has timed out while waiting for a BEGIN_INIT message", cur_alg->name.c_str());
            cur_alg->state = ALG_STATE_ERROR;
            ServerToAlg server_to_alg;
            server_to_alg.set_alg_name(cur_alg->name);
            server_to_alg.set_action(BEGIN_INIT);
            server_to_alg.set_result(ALG_FAILURE_TIMEOUT);
            send_message(cur_alg->sock_handle, server_to_alg);
          } else {
            LOG_I_A("Beginning init for Algorithm <%s>", cur_alg->name.c_str());

            if(recv_message(cur_alg->sock_handle, alg_to_server) &&
              alg_to_server.alg_name().compare(cur_alg->name) == 0 &&
              alg_to_server.action() == BEGIN_INIT) {
              
              ServerToAlg server_to_alg;
              server_to_alg.set_alg_name(alg_to_server.alg_name());
              server_to_alg.set_action(BEGIN_INIT);
              server_to_alg.set_result(ALG_SUCCESS);

              send_message(cur_alg->sock_handle, server_to_alg);
              cur_alg->state = ALG_STATE_PERFORMING_INIT;

      	      while(!can_be_read(cur_alg->sock_handle) && timeout < server_state.alg_timeout(ALG_STATE_PERFORMING_INIT)) {
                LOG_V_A("Waiting on Algorithm <%s> for an END INIT message", cur_alg->name.c_str());
                usleep(server_state.time_granularity());
                //timeout += server_state.time_granularity();
              }

      	      if(!can_be_read(cur_alg->sock_handle)) {
                LOG_E_A("Algorithm <%s> has timed out while waiting for a END INIT message", cur_alg->name.c_str());
                cur_alg->state = ALG_STATE_ERROR;
                ServerToAlg server_to_alg;
                server_to_alg.set_alg_name(cur_alg->name);
                server_to_alg.set_action(END_INIT);
                server_to_alg.set_result(ALG_FAILURE_TIMEOUT);
                send_message(cur_alg->sock_handle, server_to_alg);
              } else {
                if(recv_message(cur_alg->sock_handle, alg_to_server) && alg_to_server.alg_name().compare(cur_alg->name) == 0 &&
                  alg_to_server.action() == END_INIT) {
                  server_to_alg.set_action(END_INIT);
                  server_to_alg.set_result(ALG_SUCCESS);
                  send_message(cur_alg->sock_handle, server_to_alg);
                  cur_alg->state = ALG_STATE_READY_PRE;
                } else {
                  LOG_E_A("Algorithm <%s> sent invalid message. Expected end of init", alg_to_server.alg_name().c_str());
                  cur_alg->state = ALG_STATE_ERROR;
                  ServerToAlg server_to_alg;
                  server_to_alg.set_alg_name(alg_to_server.alg_name());
                  server_to_alg.set_action(alg_to_server.action());
                  server_to_alg.set_result(ALG_FAILURE_UNEXPECTED_MESSAGE);
                  send_message(cur_alg->sock_handle, server_to_alg);
                }
              }
            } else {
              LOG_E_A("Algorithm <%s> sent invalid message. Expected begin init", alg_to_server.alg_name().c_str());
              cur_alg->state = ALG_STATE_ERROR;
              ServerToAlg server_to_alg;
              server_to_alg.set_alg_name(alg_to_server.alg_name());
              server_to_alg.set_action(alg_to_server.action());
              server_to_alg.set_result(ALG_FAILURE_UNEXPECTED_MESSAGE);
              send_message(cur_alg->sock_handle, server_to_alg);
            }
          }
        }
      }
    }

    stop_alg_level = server_state.get_num_levels();
    for(size_t cur_level_index = 0; cur_level_index < stop_alg_level; cur_level_index++) {
      size_t stop_alg_index = server_state.get_num_algs(cur_level_index);

      int64_t timeout = 0;
      for(size_t cur_alg_index = 0; cur_alg_index < stop_alg_index; cur_alg_index++) {
        StingerAlgState * cur_alg = server_state.get_alg(cur_level_index, cur_alg_index);

        /* send init preprocessing message to each ready algorithm in this level */
        if(cur_alg->state == ALG_STATE_READY_PRE) {
          AlgToServer alg_to_server;

          while(!can_be_read(cur_alg->sock_handle) && timeout < server_state.alg_timeout(ALG_STATE_READY_PRE)) {
            usleep(server_state.time_granularity());
            //timeout += server_state.time_granularity();
          }

          if(!can_be_read(cur_alg->sock_handle)) {
            LOG_E_A("Algorithm <%s> has timed out while waiting for a BEGIN_PREPROCESS message", cur_alg->name.c_str());
            cur_alg->state = ALG_STATE_ERROR;
            ServerToAlg server_to_alg;
            server_to_alg.set_alg_name(cur_alg->name);
            server_to_alg.set_action(BEGIN_PREPROCESS);
            server_to_alg.set_result(ALG_FAILURE_TIMEOUT);
            send_message(cur_alg->sock_handle, server_to_alg);
          } else {

            if(recv_message(cur_alg->sock_handle, alg_to_server) && alg_to_server.alg_name().compare(cur_alg->name) == 0 &&
              alg_to_server.action() == BEGIN_PREPROCESS) {
              
              ServerToAlg server_to_alg;
              server_to_alg.set_alg_name(alg_to_server.alg_name());
              server_to_alg.set_action(BEGIN_PREPROCESS);
              server_to_alg.set_result(ALG_SUCCESS);
              server_to_alg.set_allocated_batch(batch);
              send_message(cur_alg->sock_handle, server_to_alg);
              server_to_alg.release_batch();
              cur_alg->state = ALG_STATE_PERFORMING_PRE;
            } else {
              LOG_E_A("Algorithm <%s> sent invalid message. Expected begin preprocess", alg_to_server.alg_name().c_str());
              cur_alg->state = ALG_STATE_ERROR;
              ServerToAlg server_to_alg;
              server_to_alg.set_alg_name(alg_to_server.alg_name());
              server_to_alg.set_action(alg_to_server.action());
              server_to_alg.set_result(ALG_FAILURE_UNEXPECTED_MESSAGE);
              send_message(cur_alg->sock_handle, server_to_alg);
            }
          }
        }
      }

      /* for each alg in this level that is processing: wait for message indicating finished */
      timeout = 0;
      for(size_t cur_alg_index = 0; cur_alg_index < stop_alg_index; cur_alg_index++) {
        StingerAlgState * cur_alg = server_state.get_alg(cur_level_index, cur_alg_index);

        if(cur_alg->state == ALG_STATE_PERFORMING_PRE) {
          AlgToServer alg_to_server;

          while(!can_be_read(cur_alg->sock_handle) && timeout < server_state.alg_timeout(ALG_STATE_PERFORMING_PRE)) {
            usleep(server_state.time_granularity());
            //timeout += server_state.time_granularity();
          }

          if(!can_be_read(cur_alg->sock_handle)) {
            LOG_E_A("Algorithm <%s> has timed out while waiting for a END_PREPROCESS message", cur_alg->name.c_str());
            cur_alg->state = ALG_STATE_ERROR;
            ServerToAlg server_to_alg;
            server_to_alg.set_alg_name(cur_alg->name);
            server_to_alg.set_action(END_PREPROCESS);
            server_to_alg.set_result(ALG_FAILURE_TIMEOUT);
            send_message(cur_alg->sock_handle, server_to_alg);
          } else {

            if(recv_message(cur_alg->sock_handle, alg_to_server) && alg_to_server.alg_name().compare(cur_alg->name) == 0 &&
              alg_to_server.action() == END_PREPROCESS) {

              ServerToAlg server_to_alg;
              server_to_alg.set_action(END_PREPROCESS);
              server_to_alg.set_result(ALG_SUCCESS);
              send_message(cur_alg->sock_handle, server_to_alg);
              cur_alg->state = ALG_STATE_READY_POST;
            } else {
              LOG_E_A("Algorithm <%s> sent invalid message. Expected end of preprocess", alg_to_server.alg_name().c_str());
              cur_alg->state = ALG_STATE_ERROR;
              ServerToAlg server_to_alg;
              server_to_alg.set_alg_name(alg_to_server.alg_name());
              server_to_alg.set_action(alg_to_server.action());
              server_to_alg.set_result(ALG_FAILURE_UNEXPECTED_MESSAGE);
            }
          }
        }
      }
    }

    /* update stinger */
    update_time = timer();
    process_batch(server_state.get_stinger(), *batch);
    update_time = timer() - update_time;
    int64_t edge_count = batch->insertions_size() + batch->deletions_size();
    LOG_I_A("Server processed %" PRId64 " edges in %20.15e seconds", edge_count, update_time);
    LOG_I_A("%f edges per second", ((double) edge_count) / update_time);

    /* update performance stats */
    S->num_insertions += batch->insertions_size();
    S->num_deletions += batch->deletions_size();
    S->num_insertions_last_batch = batch->insertions_size();
    S->num_deletions_last_batch = batch->deletions_size();
    S->update_time = update_time;
    S->queue_size = server_state.get_queue_size();

    /* loop through each algorithm, send start postprocessing message */
    stop_alg_level = server_state.get_num_levels();
    for(size_t cur_level_index = 0; cur_level_index < stop_alg_level; cur_level_index++) {
      size_t stop_alg_index = server_state.get_num_algs(cur_level_index);

      int64_t timeout = 0;
      for(size_t cur_alg_index = 0; cur_alg_index < stop_alg_index; cur_alg_index++) {
        StingerAlgState * cur_alg = server_state.get_alg(cur_level_index, cur_alg_index);

        if(cur_alg->state == ALG_STATE_READY_POST) {
          AlgToServer alg_to_server;

          while(!can_be_read(cur_alg->sock_handle) && timeout < server_state.alg_timeout(ALG_STATE_READY_POST)) {
            usleep(server_state.time_granularity());
            //timeout += server_state.time_granularity();
          }

          if(!can_be_read(cur_alg->sock_handle)) {
            LOG_E_A("Algorithm <%s> has timed out while waiting for a BEGIN_POSTPROCESS message", cur_alg->name.c_str());
            cur_alg->state = ALG_STATE_ERROR;
            ServerToAlg server_to_alg;
            server_to_alg.set_alg_name(cur_alg->name);
            server_to_alg.set_action(BEGIN_POSTPROCESS);
            server_to_alg.set_result(ALG_FAILURE_TIMEOUT);
            send_message(cur_alg->sock_handle, server_to_alg);
          } else {
            if(recv_message(cur_alg->sock_handle, alg_to_server) && alg_to_server.alg_name().compare(cur_alg->name) == 0&&
              alg_to_server.action() == BEGIN_POSTPROCESS) {
              
              ServerToAlg server_to_alg;
              server_to_alg.set_alg_name(alg_to_server.alg_name());
              server_to_alg.set_action(BEGIN_POSTPROCESS);
              server_to_alg.set_result(ALG_SUCCESS);
              server_to_alg.set_allocated_batch(batch);
              send_message(cur_alg->sock_handle, server_to_alg);
              server_to_alg.release_batch();
              cur_alg->state = ALG_STATE_PERFORMING_POST;
            } else {
              LOG_E_A("Algorithm <%s> sent invalid message. Expected begin postprocess", alg_to_server.alg_name().c_str());
              cur_alg->state = ALG_STATE_ERROR;
              ServerToAlg server_to_alg;
              server_to_alg.set_alg_name(alg_to_server.alg_name());
              server_to_alg.set_action(alg_to_server.action());
              server_to_alg.set_result(ALG_FAILURE_UNEXPECTED_MESSAGE);
              send_message(cur_alg->sock_handle, server_to_alg);
            }
          }
        }
      }

      /* TODO timeout */

      /* loop through each algorithm and wait for message indicating finished postprocesing */
      timeout = 0;
      for(size_t cur_alg_index = 0; cur_alg_index < stop_alg_index; cur_alg_index++) {
        StingerAlgState * cur_alg = server_state.get_alg(cur_level_index, cur_alg_index);

        if(cur_alg->state == ALG_STATE_PERFORMING_POST) {
          AlgToServer alg_to_server;

          while(!can_be_read(cur_alg->sock_handle) && timeout < server_state.alg_timeout(ALG_STATE_PERFORMING_POST)) {
            usleep(server_state.time_granularity());
            //timeout += server_state.time_granularity();
          }

          if(!can_be_read(cur_alg->sock_handle)) {
            LOG_E_A("Algorithm <%s> has timed out while waiting for a END_POSTPROCESS message", cur_alg->name.c_str());
            cur_alg->state = ALG_STATE_ERROR;
            ServerToAlg server_to_alg;
            server_to_alg.set_alg_name(cur_alg->name);
            server_to_alg.set_action(END_POSTPROCESS);
            server_to_alg.set_result(ALG_FAILURE_TIMEOUT);
            send_message(cur_alg->sock_handle, server_to_alg);
          } else {
            if(recv_message(cur_alg->sock_handle, alg_to_server) && alg_to_server.alg_name().compare(cur_alg->name) == 0 &&
              alg_to_server.action() == END_POSTPROCESS) {

              ServerToAlg server_to_alg;
              server_to_alg.set_action(END_POSTPROCESS);
              server_to_alg.set_result(ALG_SUCCESS);
              send_message(cur_alg->sock_handle, server_to_alg);
              cur_alg->state = ALG_STATE_READY_PRE;
            } else {
              LOG_E_A("Algorithm <%s> sent invalid message. Expected end postprocess", alg_to_server.alg_name().c_str());
              cur_alg->state = ALG_STATE_ERROR;
              ServerToAlg server_to_alg;
              server_to_alg.set_alg_name(alg_to_server.alg_name());
              server_to_alg.set_action(alg_to_server.action());
              server_to_alg.set_result(ALG_FAILURE_UNEXPECTED_MESSAGE);
              send_message(cur_alg->sock_handle, server_to_alg);
            }
          }
        }
      }
    }

    /* Look through each monitor and send an update message (using a copy of the cached message for dependencies) */
    size_t stop_mon_index = server_state.get_num_mons();
    {
      int64_t timeout = 0;
      ServerToMon * server_to_mon = server_state.get_server_to_mon_copy();
      for(size_t cur_mon_index = 0; cur_mon_index < stop_mon_index; cur_mon_index++) {
        StingerMonState * cur_mon = server_state.get_mon(cur_mon_index);

        if(cur_mon->state == MON_STATE_READY_UPDATE) {
          MonToServer mon_to_server;

          while(!can_be_read(cur_mon->sock_handle) && timeout < server_state.mon_timeout(MON_STATE_READY_UPDATE)) {
            usleep(server_state.time_granularity());
            //timeout += server_state.time_granularity();
          }

          if(!can_be_read(cur_mon->sock_handle)) {
            LOG_E_A("Monitor <%s> has timed out while waiting for a BEGIN_UPDATE message", cur_mon->name.c_str());
            cur_mon->state = MON_STATE_ERROR;
            ServerToMon server_to_mon;
            server_to_mon.set_mon_name(cur_mon->name);
            server_to_mon.set_action(BEGIN_UPDATE);
            server_to_mon.set_result(MON_FAILURE_TIMEOUT);
            send_message(cur_mon->sock_handle, server_to_mon);
          } else {

            if(recv_message(cur_mon->sock_handle, mon_to_server) && (mon_to_server.mon_name().compare(cur_mon->name) == 0) &&
              mon_to_server.action() == BEGIN_UPDATE) {
              
              server_to_mon->set_mon_name(mon_to_server.mon_name());
              server_to_mon->set_action(BEGIN_UPDATE);
              server_to_mon->set_result(MON_SUCCESS);
              server_to_mon->set_allocated_batch(batch);
              send_message(cur_mon->sock_handle, *server_to_mon);
              server_to_mon->release_batch();
              cur_mon->state = MON_STATE_PERFORMING_UPDATE;
            } else {
              LOG_E_A("Monitor <%s> sent invalid message. Expected begin postprocess", cur_mon->name.c_str());
              cur_mon->state = MON_STATE_ERROR;
              server_to_mon->set_mon_name(mon_to_server.mon_name());
              server_to_mon->set_action(mon_to_server.action());
              server_to_mon->set_result(MON_FAILURE_UNEXPECTED_MESSAGE);
              send_message(cur_mon->sock_handle, *server_to_mon);
            }
          }
        }
      }

      delete server_to_mon;
    }

    /* TODO timeout */

    /* loop through each monitor and wait for message indicating finished postprocesing */
    int64_t timeout = 0;
    for(size_t cur_mon_index = 0; cur_mon_index < stop_mon_index; cur_mon_index++) {
      StingerMonState * cur_mon = server_state.get_mon(cur_mon_index);

      if(cur_mon->state == MON_STATE_PERFORMING_UPDATE) {
        MonToServer mon_to_server;

        while(!can_be_read(cur_mon->sock_handle) && timeout < server_state.mon_timeout(MON_STATE_PERFORMING_UPDATE)) {
          usleep(server_state.time_granularity());
          //timeout += server_state.time_granularity();
        }

        if(!can_be_read(cur_mon->sock_handle)) {
          LOG_E_A("Monitor <%s> has timed out while waiting for a END_UPDATE message", cur_mon->name.c_str());
          cur_mon->state = MON_STATE_ERROR;
          ServerToMon server_to_mon;
          server_to_mon.set_mon_name(cur_mon->name);
          server_to_mon.set_action(END_UPDATE);
          server_to_mon.set_result(MON_FAILURE_TIMEOUT);
          send_message(cur_mon->sock_handle, server_to_mon);
        } else {

          if(recv_message(cur_mon->sock_handle, mon_to_server) && mon_to_server.mon_name().compare(cur_mon->name) == 0 &&
            mon_to_server.action() == END_UPDATE) {

            ServerToMon server_to_mon;
            server_to_mon.set_action(END_UPDATE);
            server_to_mon.set_result(MON_SUCCESS);
            send_message(cur_mon->sock_handle, server_to_mon);
            cur_mon->state = MON_STATE_READY_UPDATE;
          } else {
            LOG_E_A("Monitor <%s> sent invalid message. Expected end postprocess", mon_to_server.mon_name().c_str());
            cur_mon->state = MON_STATE_ERROR;
            ServerToMon server_to_mon;
            server_to_mon.set_mon_name(mon_to_server.mon_name());
            server_to_mon.set_action(mon_to_server.action());
            server_to_mon.set_result(MON_FAILURE_UNEXPECTED_MESSAGE);
          }
        }
      }
    }
    server_state.write_data();

    delete batch;
    
    batch_time = timer() - batch_time;
    LOG_I_A("Algorithm handling loop took %20.15e seconds", batch_time);
    S->batch_time = batch_time;
  }
}

void *
start_alg_handling(void *)
{
  StingerServerState & server_state = StingerServerState::get_server_state();

  int port_algs = server_state.get_port_algs();

  LOG_D("Opening the socket");
  int sock_handle = listen_for_client(port_algs);

  LOG_V_A("STINGER alg server listening for input on port %d", port_algs);

  pthread_t main_loop_thread;
  pthread_create(&main_loop_thread, NULL, &process_loop_handler, NULL);
  server_state.set_main_loop_thread(main_loop_thread);

  /* ENABLE ME for a quick empty batch test */
  /*
  pthread_t test_thread;
  pthread_create(&test_thread, NULL, &test_thread_handler, NULL);
  */

  while(1) {
    struct AcceptedSock * accepted_sock = (struct AcceptedSock *)xcalloc(1,sizeof(struct AcceptedSock));

    LOG_D("Waiting for connections...")
    accepted_sock->handle = accept(sock_handle, &(accepted_sock->addr), &(accepted_sock->len));

    pthread_t new_thread;
    pthread_create(&new_thread, NULL, &new_connection_handler, (void *)accepted_sock);
    server_state.push_thread(new_thread);
//    close(accepted_sock->handle);
  }
}

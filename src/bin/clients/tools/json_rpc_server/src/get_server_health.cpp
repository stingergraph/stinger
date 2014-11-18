#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "stinger_core/stinger.h"
#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"
#include "json_rpc_server.h"
#include "json_rpc.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;

static int get_loadavg (rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
static int get_time (rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
static int get_rpc_methods (rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);
static int get_uptime (rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator);


int64_t 
JSON_RPC_get_server_health::operator()(rapidjson::Value * params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  rpc_params_t p[] = {
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (!contains_params(p, params)) {
    return json_rpc_error(-32602, result, allocator);
  }

  stinger_t * S = server_state->get_stinger();
  if (!S) {
    LOG_E ("STINGER pointer is invalid");
    return json_rpc_error(-32603, result, allocator);
  }

  struct stinger_fragmentation_t * stats = (struct stinger_fragmentation_t *) xmalloc(sizeof(struct stinger_fragmentation_t));
  stinger_fragmentation (S, S->max_nv, stats);

  /* Hostname */
  char buf[256];
  gethostname(buf, 127);
  rapidjson::Value hostname;
  hostname.SetString(buf, strlen(buf), allocator);
  result.AddMember("host", hostname, allocator);

  /* Process ID */
  pid_t myPid = getpid();
  rapidjson::Value pid;
  pid.SetInt64(myPid);
  result.AddMember("pid", pid, allocator);

  /* Uptime */
  get_uptime(result, allocator);
  
  /* Local Time */
  get_time(result, allocator);

  /* Most recent timestamp */
  rapidjson::Value max_time_seen;
  max_time_seen.SetInt64(server_state->get_max_time());
  result.AddMember("time", max_time_seen, allocator);

  /* Number of vertices */
  rapidjson::Value nv;
  nv.SetInt64(stinger_num_active_vertices(S));
  result.AddMember("vertices", nv, allocator);

  /* Maximum number of vertices */
  rapidjson::Value max_nv;
  max_nv.SetInt64(S->max_nv);
  result.AddMember("max_vertices", max_nv, allocator);

  /* Number of directed edges */
  rapidjson::Value ne;
  ne.SetInt64(stinger_total_edges(S));
  result.AddMember("edges", ne, allocator);

  /* Edges per block */
  rapidjson::Value edges_per_block;
  edges_per_block.SetInt64(STINGER_EDGEBLOCKSIZE);
  result.AddMember("max_edges_per_block", edges_per_block, allocator);

  /* Number of edge blocks in use */
  rapidjson::Value edge_blocks_in_use;
  edge_blocks_in_use.SetInt64(stats->edge_blocks_in_use - 1);
  result.AddMember("edge_blocks_in_use", edge_blocks_in_use, allocator);

  /* Maximum number of edge blocks */
  rapidjson::Value max_neb;
  max_neb.SetInt64(S->max_neblocks);
  result.AddMember("max_neblocks", max_neb, allocator);

  /* Number of fragmented edge blocks */
  rapidjson::Value num_fragmented_blocks;
  num_fragmented_blocks.SetInt64(stats->num_fragmented_blocks);
  result.AddMember("num_fragmented_blocks", num_fragmented_blocks, allocator);

  /* Number of holes */
  rapidjson::Value num_empty_edges;
  num_empty_edges.SetInt64(stats->num_empty_edges);
  result.AddMember("num_empty_edges", num_empty_edges, allocator);

  /* Number of empty edge blocks */
  rapidjson::Value num_empty_blocks;
  num_empty_blocks.SetInt64(stats->num_empty_blocks);
  result.AddMember("num_empty_blocks", num_empty_blocks, allocator);

  /* Edge block fill percentage */
  rapidjson::Value fill_percent;
  fill_percent.SetDouble(stats->fill_percent);
  result.AddMember("fill_percent", fill_percent, allocator);

  /* Average number of edges per block */
  rapidjson::Value avg_number_of_edges;
  avg_number_of_edges.SetDouble(stats->avg_number_of_edges);
  result.AddMember("avg_number_of_edges", avg_number_of_edges, allocator);

  /* Number of edge types */
  rapidjson::Value num_etypes;
  int64_t num_edge_types = stinger_etype_names_count(S);
  num_etypes.SetInt64(num_edge_types);
  result.AddMember("num_etypes", num_etypes, allocator);

  /* Edge type strings */
  rapidjson::Value edge_types, type_name;
  edge_types.SetArray();
  for (int64_t i = 0; i < num_edge_types; i++) {
    char * type_name_str = stinger_etype_names_lookup_name(S, i);
    type_name.SetString(type_name_str, strlen(type_name_str), allocator);
    edge_types.PushBack(type_name, allocator);
  }
  result.AddMember("edge_types", edge_types, allocator);

  /* Maximum number of edge types */
  rapidjson::Value max_num_etypes;
  max_num_etypes.SetInt64(S->max_netypes);
  result.AddMember("max_num_etypes", max_num_etypes, allocator);

  /* Consistency check */
  /* TODO: The consistency check can inadvertently return an error code here
   * because the data structure may be changing underneath.  Consistency check
   * should only be called when the underlying graph is quiet.  Or maybe we can
   * fix the way memory mapping works.
   */
  //uint64_t max_active_nv = stinger_max_active_vertex (S);
  //uint32_t check = stinger_consistency_check (S, max_active_nv+1);
  //rapidjson::Value consistency_check;
  //consistency_check.SetInt64(check);
  //result.AddMember("consistency_check", consistency_check, allocator);

  /* Uptime load averages */
  get_loadavg(result, allocator);

  /* Algorithms registered */


  /* JSON RPC Methods */
  get_rpc_methods(result, allocator);


  /* SC14 performance stats */
  rapidjson::Value num_insertions;
  num_insertions.SetInt64(S->num_insertions);
  result.AddMember("num_insertions", num_insertions, allocator);

  rapidjson::Value num_deletions;
  num_deletions.SetInt64(S->num_deletions);
  result.AddMember("num_deletions", num_deletions, allocator);

  rapidjson::Value num_insertions_last_batch;
  num_insertions_last_batch.SetInt64(S->num_insertions_last_batch);
  result.AddMember("num_insertions_last_batch", num_insertions_last_batch, allocator);

  rapidjson::Value num_deletions_last_batch;
  num_deletions_last_batch.SetInt64(S->num_deletions_last_batch);
  result.AddMember("num_deletions_last_batch", num_deletions_last_batch, allocator);

  rapidjson::Value batch_time;
  batch_time.SetDouble(S->batch_time);
  result.AddMember("batch_time", batch_time, allocator);

  rapidjson::Value update_time;
  update_time.SetDouble(S->update_time);
  result.AddMember("update_time", update_time, allocator);

  rapidjson::Value queue_size;
  queue_size.SetInt64(S->queue_size);
  result.AddMember("queue_size", queue_size, allocator);

  rapidjson::Value dropped_batches;
  dropped_batches.SetInt64(S->dropped_batches);
  result.AddMember("dropped_batches", dropped_batches, allocator);


  free(stats);

  return 0;
}


static int
get_loadavg (rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  std::ifstream myfile;
  std::string line;
  std::string::size_type sz;
  rapidjson::Value loadAvg (rapidjson::kArrayType);
  rapidjson::Value loadAvgValue;
  
  myfile.open ("/proc/loadavg");

  if (!myfile.is_open()) {
    return -1;
  }

  if (getline(myfile, line)) {
    size_t pos = 0;
    double oneMin = (double) atof(line.c_str());
    loadAvgValue.SetDouble(oneMin);
    loadAvg.PushBack(loadAvgValue, allocator);

    pos = line.find(' ', pos);
    line = line.substr(pos+1);
    double fiveMin = (double) atof(line.c_str());
    loadAvgValue.SetDouble(fiveMin);
    loadAvg.PushBack(loadAvgValue, allocator);

    pos = line.find(' ', pos);
    line = line.substr(pos+1);
    double fifteenMin = (double) atof(line.c_str());
    loadAvgValue.SetDouble(fifteenMin);
    loadAvg.PushBack(loadAvgValue, allocator);

    result.AddMember("loadavg", loadAvg, allocator);
  }

  myfile.close();
  return 0;
}

static int
get_time (rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  time_t curtime;
  time(&curtime);

  char buf[50];
  ctime_r(&curtime, buf);

  rapidjson::Value local_time;
  local_time.SetString(buf, 24, allocator);
  result.AddMember("local_time", local_time, allocator);

  return 0;
}

static int
get_rpc_methods (rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  std::map<std::string, JSON_RPCFunction *>::iterator it;
  rapidjson::Value methods(rapidjson::kArrayType);
  rapidjson::Value method_name;

  JSON_RPCServerState & server_state = JSON_RPCServerState::get_server_state();
  for (it = server_state.rpc_function_begin(); it != server_state.rpc_function_end(); ++it) {
    method_name.SetString(it->first.c_str(), strlen(it->first.c_str()), allocator);
    methods.PushBack(method_name, allocator);
  }

  result.AddMember("rpc_methods", methods, allocator);
}

static int
get_uptime (rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
{
  JSON_RPCServerState & server_state = JSON_RPCServerState::get_server_state();
  time_t t = server_state.get_time_since_start();

  rapidjson::Value uptime;
  uptime.SetInt64(t);
  result.AddMember("uptime", uptime, allocator);

  return 0;
}

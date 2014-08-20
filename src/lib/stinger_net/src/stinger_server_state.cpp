#include "stinger_server_state.h"

extern "C" {
  #include "stinger_core/stinger_error.h"
  #include "stinger_core/x86_full_empty.h"
}

#include <unistd.h>
#include <algorithm>

using namespace gt::stinger;

template<typename T>
struct delete_functor
{
  void operator() (T * item)
  {
    delete item;
  }
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * PRIVATE METHODS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
StingerServerState::StingerServerState() : port_names(10101), port_streams(10102), port_algs(10103),
				    convert_num_to_string(1), batch_count(0),
				    alg_lock(1), stream_lock(1), batch_lock(1), dep_lock(1), mon_lock(1),
				    write_alg_data(false), write_names(false), history_cap(0), out_dir("./"),
				    stinger_sz(0)
{
  LOG_D("Initializing server state.");


  /* Setup timeout lengths (all in microsecs) */
  for(int64_t i = 0; i < ALG_STATE_MAX; i++) {
    alg_timeouts[i] = 0;
  }
  for(int64_t i = 0; i < MON_STATE_MAX; i++) {
    mon_timeouts[i] = 0;
  }

  alg_timeouts[ALG_STATE_READY_INIT]        =  5000000;
  alg_timeouts[ALG_STATE_PERFORMING_INIT]   = 10000000;
  alg_timeouts[ALG_STATE_READY_PRE]         =  5000000;
  alg_timeouts[ALG_STATE_PERFORMING_PRE]    =  9000000;
  alg_timeouts[ALG_STATE_READY_POST]        =  5000000;
  alg_timeouts[ALG_STATE_PERFORMING_POST]   =  9000000;

  mon_timeouts[MON_STATE_READY_UPDATE]      = 10000000;
  mon_timeouts[MON_STATE_PERFORMING_UPDATE] =  9000000;

  timeout_granularity = 100;
}

StingerServerState::~StingerServerState()
{
  LOG_D("Entering StingerServerState destructor");
  std::for_each(algs.begin(), algs.end(), delete_functor<StingerAlgState>());
  std::for_each(streams.begin(), streams.end(), delete_functor<StingerStreamState>());
  LOG_D("Leaving StingerServerState destructor");
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *
 * PUBLIC METHODS
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
StingerServerState &
StingerServerState::get_server_state()
{
  LOG_D("called");
  static StingerServerState self;
  return self;
}

/**
* @brief Get the current names port number.
*
* @return The current names port number.
*/
int
StingerServerState::get_port_names() 
{
  LOG_D_A("returning %ld", (long) port_names);
  return port_names;
}

/**
* @brief Get the current streams port number.
*
* @return The current streams port number.
*/
int
StingerServerState::get_port_streams() 
{
  LOG_D_A("returning %ld", (long) port_streams);
  return port_streams;
}

/**
* @brief Get the current algs port number.
*
* @return The current algs port number.
*/
int
StingerServerState::get_port_algs() 
{
  LOG_D_A("returning %ld", (long) port_algs);
  return port_algs;
}

/**
* @brief Sets the ports to be used by the server. 
* 
* NOTE: Should only be called during initialization - i.e. when parsing
* options.  This will not change the port once the server is running.
*
* @param new_port_names The new port number for the graph name server (1 - 65535, but really 1024 ~ 35000 or so).
* @param new_port_streams The new port number for streams to connect to (1 - 65535, but really 1024 ~ 35000 or so).
* @param new_port_algs The new port number for algorithms to connect to (1 - 65535, but really 1024 ~ 35000 or so).
*
* @return 0 on success, -1 on failure.
*/
int
StingerServerState::set_port(int new_port_names, int new_port_streams, int new_port_algs)
{
  LOG_D_A("called with %ld, %ld, %ld", (long) new_port_names, (long) new_port_streams, (long) new_port_algs);

  if (new_port_names > 0 && new_port_names < 65535) {
    port_names = new_port_names;
  } else {
    LOG_W_A("New names port number %ld is invalid", (long) new_port_names);
    return -1;
  }

  if (new_port_streams > 0 && new_port_streams < 65535) {
    port_streams = new_port_streams;
  } else {
    LOG_W_A("New streams port number %ld is invalid", (long) new_port_streams);
    return -1;
  }
  
  if (new_port_algs > 0 && new_port_algs < 65535) {
    port_algs = new_port_algs;
  } else {
    LOG_W_A("New algs port number %ld is invalid", (long) new_port_algs);
    return -1;
  }

  return 0;
}

/**
* @brief Indicates whether or not the server should convert stinger IDs to strings
* when applying a batch so that algorithms have the string information in the post
* processing stage.
*
* @return 0 - false, otherwise - true
*/
int
StingerServerState::convert_numbers_only_to_strings() {
  return convert_num_to_string;
}

/**
* @brief Indicates whether or not the server should convert stinger IDs to strings
* when applying a batch so that algorithms have the string information in the post
* processing stage.
*
* @param new_value 0 - Do not convert, otherwise - Convert
*
* @return The input value
*/
int
StingerServerState::set_convert_numbers_only_to_strings(int new_value) {
  return convert_num_to_string = new_value;
}

/**
* @brief Atomically enqueue a received batch.
*
* @param batch A pointer to the batch to be enqueued (DO NOT DELETE)
*/
void
StingerServerState::enqueue_batch(StingerBatch * batch)
{
  LOG_D_A("%p %ld insertions %ld deletions: Attempt to acquire lock", batch, (long) batch->insertions_size(), (long) batch->deletions_size());
  readfe((uint64_t *)&batch_lock);
  LOG_D_A("%p %ld insertions %ld deletions: Lock acquired, enqueueing", batch, (long) batch->insertions_size(), (long) batch->deletions_size());
  batches.push(batch);
  writeef((uint64_t *)&batch_lock, 1);
  LOG_D_A("%p %ld insertions %ld deletions: Lock released", batch, (long) batch->insertions_size(), (long) batch->deletions_size());
}

/**
* @brief Blocking call to atomically dequeue a received batch.
*
* Blocks until the queue is not empty.
*
* @return A pointer to the batch (YOU MUST HANDLE DELETION)
*/
StingerBatch *
StingerServerState::dequeue_batch()
{
  StingerBatch * rtn = NULL;

  LOG_D("Attempt to acquire lock")
  while(1) {
    readfe((uint64_t *)&batch_lock);
    if(!batches.empty()) {
      rtn = batches.front();
      batches.pop();
      writeef((uint64_t *)&batch_lock, 1);
      break;
    } else {
      writeef((uint64_t *)&batch_lock, 1);
      usleep(100);
    }
  }

  batch_count++;

  return rtn;
}

size_t
StingerServerState::get_num_streams()
{
  size_t num = 0;
  readfe((uint64_t *)&stream_lock);
  num = streams.size();
  writeef((uint64_t *)&stream_lock, 1);
  return num;
}

StingerStreamState *
StingerServerState::get_stream(size_t index)
{
  StingerStreamState * rtn = NULL;
  readfe((uint64_t *)&stream_lock);
  rtn = streams[index];
  writeef((uint64_t *)&stream_lock, 1);
  return rtn;
}

StingerStreamState *
StingerServerState::get_stream(std::string & name)
{
  StingerStreamState * rtn = NULL;
  readfe((uint64_t *)&stream_lock);
  rtn = stream_map[name];
  writeef((uint64_t *)&stream_lock, 1);
  return rtn;
}

size_t
StingerServerState::get_num_levels()
{
  size_t num = 0;
  readfe((uint64_t *)&alg_lock);
  num = alg_tree.size();
  writeef((uint64_t *)&alg_lock, 1);
  return num;
}

size_t
StingerServerState::get_num_algs()
{
  size_t num = 0;
  readfe((uint64_t *)&alg_lock);
  num = algs.size();
  writeef((uint64_t *)&alg_lock, 1);
  return num;
}

size_t
StingerServerState::get_num_algs(size_t level)
{
  size_t num = 0;
  readfe((uint64_t *)&alg_lock);
  num = alg_tree[level].size();
  writeef((uint64_t *)&alg_lock, 1);
  return num;
}

StingerAlgState *
StingerServerState::get_alg(size_t num)
{
  StingerAlgState * rtn = NULL;
  readfe((uint64_t *)&alg_lock);
  rtn = algs[num];
  writeef((uint64_t *)&alg_lock, 1);
  return rtn;
}

StingerAlgState *
StingerServerState::get_alg(size_t level, size_t index)
{
  StingerAlgState * rtn = NULL;
  readfe((uint64_t *)&alg_lock);
  rtn = alg_tree[level][index];
  writeef((uint64_t *)&alg_lock, 1);
  return rtn;
}

StingerAlgState *
StingerServerState::get_alg(const std::string & name)
{
  StingerAlgState * rtn = NULL;
  readfe((uint64_t *)&alg_lock);
  rtn = alg_map[name];
  writeef((uint64_t *)&alg_lock, 1);
  return rtn;
}

size_t
StingerServerState::add_alg(size_t level, StingerAlgState * alg)
{
  size_t rtn = 0;
  readfe((uint64_t *)&alg_lock);
  alg_tree.resize(level+1);
  alg_tree[level].push_back(alg);
  alg_map[alg->name] = alg;
  rtn = algs.size();
  algs.push_back(alg);
  writeef((uint64_t *)&alg_lock, 1);

  readfe((uint64_t *)&mon_lock);
  server_to_mon.add_dep_name(alg->name);
  server_to_mon.add_dep_data_loc(alg->data_loc);
  server_to_mon.add_dep_description(alg->data_description);
  server_to_mon.add_dep_data_per_vertex(alg->data_per_vertex);
  writeef((uint64_t *)&mon_lock, 1);
  return rtn;
}

bool
StingerServerState::has_alg(const std::string & name)
{
  bool rtn = false;
  readfe((uint64_t *)&alg_lock);
  rtn = alg_map.count(name) > 0;
  writeef((uint64_t *)&alg_lock, 1);
  return rtn;
}

bool
StingerServerState::delete_alg(const std::string & name)
{
  if(has_alg(name)) {
    readfe((uint64_t *)&alg_lock);
    for(int64_t i = 0; i < alg_tree.size(); i++) {
      for(int64_t j = 0; j < alg_tree[i].size(); j++) {
	if(0 == strcmp(alg_tree[i][j]->name.c_str(), name.c_str())) {
	  alg_tree[i].erase(alg_tree[i].begin() + j);
	}
      }
    }
    delete alg_map[name];
    alg_map.erase(name);
    writeef((uint64_t *)&alg_lock, 1);
    return true;
  } else {
    return false;
  }
}

size_t
StingerServerState::get_num_mons()
{
  size_t num = 0;
  readfe((uint64_t *)&mon_lock);
  num = monitors.size();
  writeef((uint64_t *)&mon_lock, 1);
  return num;
}

StingerMonState *
StingerServerState::get_mon(size_t num)
{
  StingerMonState * rtn = NULL;
  readfe((uint64_t *)&mon_lock);
  rtn = monitors[num];
  writeef((uint64_t *)&mon_lock, 1);
  return rtn;
}

StingerMonState *
StingerServerState::get_mon(const std::string & name)
{
  StingerMonState * rtn = NULL;
  readfe((uint64_t *)&mon_lock);
  rtn = monitor_map[name];
  writeef((uint64_t *)&mon_lock, 1);
  return rtn;
}

size_t
StingerServerState::add_mon(StingerMonState * mon)
{
  size_t rtn = 0;
  readfe((uint64_t *)&mon_lock);
  monitor_map[mon->name] = mon;
  rtn = monitors.size();
  monitors.push_back(mon);
  writeef((uint64_t *)&mon_lock, 1);
  return rtn;
}

bool
StingerServerState::has_mon(const std::string & name)
{
  bool rtn = false;
  readfe((uint64_t *)&mon_lock);
  rtn = monitor_map.count(name) > 0;
  writeef((uint64_t *)&mon_lock, 1);
  return rtn;
}

bool
StingerServerState::delete_mon(const std::string & name)
{
  if(has_mon(name)) {
    readfe((uint64_t *)&mon_lock);
    for(int64_t i = 0; i < monitors.size(); i++) {
      if(0 == strcmp(monitors[i]->name.c_str(), name.c_str())) {
	monitors.erase(monitors.begin() + i);
	break;
      }
    }
    delete monitor_map[name];
    monitor_map.erase(name);
    writeef((uint64_t *)&mon_lock, 1);
    return true;
  } else {
    return false;
  }
}

void
StingerServerState::set_mon_stinger(std::string loc, int64_t size) {
  readfe((uint64_t *)&mon_lock);
  server_to_mon.set_stinger_loc(loc);
  server_to_mon.set_stinger_size(size);
  writeef((uint64_t *)&mon_lock, 1);
}


ServerToMon *
StingerServerState::get_server_to_mon_copy() {
  readfe((uint64_t *)&mon_lock);
  ServerToMon * rtn = new ServerToMon(server_to_mon);
  writeef((uint64_t *)&mon_lock, 1);
  return rtn;
}

pthread_t &
StingerServerState::push_thread(pthread_t & thread)
{
  threads.push_back(thread);
  return thread;
}

void
StingerServerState::set_main_loop_thread(pthread_t thread)
{
  main_loop = thread;
}

void
StingerServerState::set_stinger(stinger_t * S)
{
  stinger = S;
}

stinger_t *
StingerServerState::get_stinger()
{
  return stinger;
}

const std::string &
StingerServerState::get_stinger_loc()
{
  return stinger_loc;
}

void
StingerServerState::set_stinger_loc(const std::string & loc)
{
  stinger_loc = loc;
}

void
StingerServerState::set_stinger_sz(size_t graph_sz)
{
  stinger_sz = graph_sz;
}

size_t
StingerServerState::get_stinger_sz()
{
  return stinger_sz;
}

int64_t
StingerServerState::time_granularity()
{
  return timeout_granularity;
}

int64_t
StingerServerState::alg_timeout(int64_t which)
{
  if(which < ALG_STATE_MAX)
    return alg_timeouts[which];
  else
    return 0;
}

int64_t
StingerServerState::mon_timeout(int64_t which)
{
  if(which < MON_STATE_MAX)
    return mon_timeouts[which];
  else
    return 0;
}

bool
StingerServerState::set_write_alg_data(bool write)
{
  return write_alg_data = write;
}

int64_t
StingerServerState::set_history_cap(int64_t hist)
{
  return history_cap = hist;
}

const char *
StingerServerState::set_out_dir(const char * out)
{
  out_dir = out;
  return out;
}

bool
StingerServerState::set_write_names(bool write)
{
  return write_names = write;
}

/* write out the current algorithm states and names as needed 
 * alg state data format (binary):
 *  64-bit integer: name_length
 *  string: name
 *  64-bit integer: description_length
 *  string: description (see stinger_alg.h for explanation)
 *  64-bit integer: maximum number of vertices
 *  64-bit integer: number of vertices in file
 *  for each field in the description
 *    [ array of number of vertices elements of the given field type ]
 */
void
StingerServerState::write_data()
{
  char name_buf[1024];
  if(write_names) {
    /* write current vertex names to file */
    snprintf(name_buf, 1024, "%s/vertex_names.%ld.vtx", out_dir.c_str(), batch_count);

    FILE * fp = fopen(name_buf, "w");
    stinger_names_save(stinger_physmap_get(stinger), fp);
    fflush(fp);
    fclose(fp);

    /* remove previous vertex names */
    snprintf(name_buf, 1024, "%s/vertex_names.%ld.vtx", out_dir.c_str(), batch_count-1);
    if(access(name_buf, F_OK) != -1) {
      unlink(name_buf);
    }
  }

  /* write out each algorithm's data */
  if(write_alg_data) {
    for(int64_t i = 0; i < get_num_algs(); i++) {
      StingerAlgState * alg = get_alg(i);

      if(alg->state >= ALG_STATE_DONE) /* skip invalid, completed, etc. */
	continue;

      snprintf(name_buf, 1024, "%s/%s.%ld.rslt", out_dir.c_str(), alg->name.c_str(), batch_count);

      int64_t nv_max = stinger->max_nv;
      int64_t nv = stinger_mapping_nv(stinger);
      int64_t size = 0;

      FILE * fp = fopen(name_buf, "w");
      
      size = alg->name.length();
      fwrite(&size, sizeof(int64_t), 1, fp);
      fwrite(alg->name.c_str(), size, sizeof(char), fp);
      size = alg->data_description.length();
      fwrite(&size, sizeof(int64_t), 1, fp);
      fwrite(alg->data_description.c_str(), size, sizeof(char), fp);
      fwrite(&nv_max, sizeof(int64_t), 1, fp);
      fwrite(&nv, sizeof(int64_t), 1, fp);

      const char * field = alg->data_description.c_str();
      uint8_t * data = (uint8_t *)alg->data;
      bool done = false;
      while(!done) {
	switch(*field) {

	  case 'f': {
	    fwrite(data, sizeof(float), nv_max, fp);
	    data += (sizeof(float) * nv_max);
	  } break;
	  case 'd': {
	    fwrite(data, sizeof(double), nv_max, fp);
	    data += (sizeof(double) * nv_max);
	  } break;
	  case 'i': {
	    fwrite(data, sizeof(int32_t), nv_max, fp);
	    data += (sizeof(int32_t) * nv_max);
	  } break;
	  case 'l': {
	    fwrite(data, sizeof(int64_t), nv_max, fp);
	    data += (sizeof(int64_t) * nv_max);
	  } break;
	  case 'b': {
	    fwrite(data, sizeof(uint8_t), nv_max, fp);
	    data += (sizeof(uint8_t) * nv_max);
	  } break;

	  default: {
	    done = true;
	  } break;
	}
	field++;
      }

      fflush(fp);
      fclose(fp);

      /* if there is a cap on history, erase (current - history) if it exists */
      if(history_cap) {
	snprintf(name_buf, 1024, "%s/%s.%ld.rslt", out_dir.c_str(), alg->name.c_str(), batch_count - history_cap);
	if(access(name_buf, F_OK) != -1) {
	  unlink(name_buf);
	}
      }
    }
  }
}

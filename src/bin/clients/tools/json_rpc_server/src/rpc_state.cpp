#include <pthread.h>
#include <sys/time.h>

#include "stinger_core/xmalloc.h"
#include "stinger_core/x86_full_empty.h"
#include "stinger_core/stinger_shared.h"
#include "rpc_state.h"
#include "stinger_core/stinger.h"


using namespace gt::stinger;

JSON_RPCServerState &
JSON_RPCServerState::get_server_state()
{
  static JSON_RPCServerState state;
  return state;
}

JSON_RPCServerState::JSON_RPCServerState() :
  next_session_id(1), session_lock(0),
  max_sessions(20), StingerMon()
{
}

JSON_RPCServerState::~JSON_RPCServerState()
{
}

void
JSON_RPCServerState::add_rpc_function(std::string name, JSON_RPCFunction * func)
{
  LOG_D_A("Adding RPC function %s", name.c_str());
  if(func)
    function_map[name] = func;
}

JSON_RPCFunction *
JSON_RPCServerState::get_rpc_function(std::string name)
{
  return function_map[name];
}

bool
JSON_RPCServerState::has_rpc_function(std::string name)
{
  return function_map.count(name) > 0;
}

void
JSON_RPCServerState::add_rpc_session(std::string name, JSON_RPCSession * func)
{
  LOG_D_A("Adding RPC session type %s", name.c_str());
  if(func)
    session_map[name] = func;
}

JSON_RPCSession *
JSON_RPCServerState::get_rpc_session(std::string name)
{
  return session_map[name];
}

bool
JSON_RPCServerState::has_rpc_session(std::string name)
{
  return session_map.count(name) > 0;
}

void
JSON_RPCServerState::update_algs(stinger_t * stinger_copy, std::string new_loc, int64_t new_sz,
                                 std::vector<StingerAlgState *> * new_algs, std::map<std::string, StingerAlgState *> * new_alg_map,
                                 const StingerBatch & batch)
{
  StingerMon::update_algs(stinger_copy, new_loc, new_sz, new_algs, new_alg_map, batch);

  readfe((uint64_t *)&session_lock);
  for(std::map<int64_t, JSON_RPCSession *>::iterator tmp = active_session_map.begin(); tmp != active_session_map.end(); tmp++)
  {
    JSON_RPCSession * session = tmp->second;
    session->lock();
    if (session->is_timed_out())
    {
      int64_t session_id = session->get_session_id();
      LOG_D_A ("Session %ld timed out. Destroying...", (long) session_id);
      delete active_session_map[session_id];
      active_session_map.erase (session_id);
    }
    else
    {
      session->update(batch);
      session->unlock();
    }
  }
  writeef((uint64_t *)&session_lock, 0);
}

bool
JSON_RPCFunction::contains_params(rpc_params_t * p, rapidjson::Value * params)
{
  if (!params)
    return true; //shouldn't this be return false?

  if (params->IsArray())
    return false;

  while(p->name)
  {
    if(!params->HasMember(p->name))
    {
      if(p->optional)
      {
        switch(p->type)
        {
        case TYPE_INT64:
        {
          *((int64_t *)p->output) = p->def;
        }
        break;
        case TYPE_STRING:
        {
          *((char **)p->output) = (char *) p->def;
        }
        break;
        case TYPE_DOUBLE:
        {
          *((double *)p->output) = (double) p->def;
        }
        break;
        case TYPE_BOOL:
        {
          *((bool *)p->output) = (bool) p->def;
        }
        break;
        case TYPE_ARRAY:
        {
          params_array_t * ptr = (params_array_t *) p->output;
          ptr->len = 0;
          ptr->arr = NULL;
        }
        break;
        }
      }
      else
      {
        return false;
      }
    }
    else
    {

      stinger_t * S = server_state->get_stinger();
      switch(p->type)
      {
      case TYPE_VERTEX:
      {
        if((*params)[p->name].IsInt64())
        {
          int64_t tmp = (*params)[p->name].GetInt64();
          if (tmp < 0 || tmp >= STINGER_MAX_LVERTICES)
            return false;
          *((int64_t *)p->output) = tmp;
        }
        else if((*params)[p->name].IsString())
        {
          int64_t tmp = stinger_mapping_lookup(S, (*params)[p->name].GetString(), (*params)[p->name].GetStringLength());
          if (tmp == -1)
            return false;
          *((int64_t *)p->output) = tmp;
        }
        else
        {
          return false;
        }
      }
      break;
      case TYPE_INT64:
      {
        if(!(*params)[p->name].IsInt64())
        {
          return false;
        }
        *((int64_t *)p->output) = (*params)[p->name].GetInt64();
      }
      break;
      case TYPE_STRING:
      {
        if(!(*params)[p->name].IsString())
        {
          return false;
        }
        *((char **)p->output) = (char *) (*params)[p->name].GetString();
      }
      break;
      case TYPE_DOUBLE:
      {
        if(!(*params)[p->name].IsDouble())
        {
          return false;
        }
        *((double *)p->output) = (*params)[p->name].GetDouble();
      }
      break;
      case TYPE_BOOL:
      {
        if(!(*params)[p->name].IsBool())
        {
          return false;
        }
        *((bool *)p->output) = (*params)[p->name].GetBool();
      }
      break;
      case TYPE_ARRAY:
      {
        if(!(*params)[p->name].IsArray())
        {
          return false;
        }
        params_array_t * ptr = (params_array_t *) p->output;
        ptr->len = (*params)[p->name].Size();
        ptr->arr = (int64_t *) xmalloc(sizeof(int64_t) * ptr->len);
        for (int64_t i = 0; i < ptr->len; i++)
        {
          if ((*params)[p->name][i].IsInt64())
          {
            int64_t tmp = (*params)[p->name][i].GetInt64();
            if (tmp < 0 || tmp >= STINGER_MAX_LVERTICES)
            {
              return false;
            }
            ptr->arr[i] = tmp;
          }
          else if ((*params)[p->name][i].IsString())
          {
            int64_t tmp = stinger_mapping_lookup(S, (*params)[p->name][i].GetString(), (*params)[p->name][i].GetStringLength());
            if (tmp == -1)
            {
              return false;
            }
            ptr->arr[i] = tmp;
          }
        }
      }
      break;
      }
    }
    p++;
  }
  return true;
}

void
JSON_RPCSession::lock()
{
  readfe((uint64_t *)&the_lock);
}

void
JSON_RPCSession::unlock()
{
  writeef((uint64_t *)&the_lock, 0);
}

bool
JSON_RPCSession::is_timed_out()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec > last_touched + 30;
}

int64_t
JSON_RPCSession::reset_timeout()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return last_touched = tv.tv_sec;
}

int64_t
JSON_RPCSession::get_session_id()
{
  return session_id;
}

int64_t
JSON_RPCSession::get_time_since()
{
  return last_touched;
}

params_array_t::params_array_t():arr(NULL) {}

params_array_t::~params_array_t()
{
  if (arr)
    free (arr);
}

int64_t
JSON_RPCServerState::get_next_session()
{
  readfe((uint64_t *)&session_lock);
  int64_t rtn = next_session_id++;
  writeef((uint64_t *)&session_lock, 0);
  return rtn;
}

int64_t
JSON_RPCServerState::add_session(int64_t session_id, JSON_RPCSession * session)
{
  readfe((uint64_t *)&session_lock);
  if (active_session_map.size() < max_sessions)
  {
    active_session_map.insert( std::pair<int64_t, JSON_RPCSession *>(session_id, session) );
  }
  else
  {
    session_id = -1;
  }
  writeef((uint64_t *)&session_lock, 0);
  return session_id;
}

int64_t
JSON_RPCServerState::destroy_session(int64_t session_id)
{
  readfe((uint64_t *)&session_lock);
  delete active_session_map[session_id];
  int64_t rtn = active_session_map.erase (session_id);
  writeef((uint64_t *)&session_lock, 0);
  return rtn;
}

int64_t
JSON_RPCServerState::get_num_sessions()
{
  readfe((uint64_t *)&session_lock);
  int64_t rtn = active_session_map.size();
  writeef((uint64_t *)&session_lock, 0);
  return rtn;
}

JSON_RPCSession *
JSON_RPCServerState::get_session(int64_t session_id)
{
  readfe((uint64_t *)&session_lock);
  std::map<int64_t, JSON_RPCSession *>::iterator tmp = active_session_map.find(session_id);
  writeef((uint64_t *)&session_lock, 0);

  if (tmp == active_session_map.end())
    return NULL;
  else
    return tmp->second;
}

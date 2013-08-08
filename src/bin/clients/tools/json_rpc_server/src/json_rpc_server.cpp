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
  LOG_D("Receiving request");
  const struct mg_request_info *request_info = mg_get_request_info(conn);

  if (strncmp(request_info->uri, "/data/", 6)==0) {
    return 0;
  }

  if (strncmp(request_info->uri, "/jsonrpc", 8)==0) {
    uint8_t * storage = (uint8_t *)xmalloc(MAX_REQUEST_SIZE);

    int64_t read = mg_read(conn, storage, MAX_REQUEST_SIZE);

    LOG_D_A("Parsing request:\n%.*s", read, storage);
    rapidjson::Document input, output;
    input.ParseInsitu<0>((char *)storage);

    json_rpc_process_request(input, output);

    rapidjson::StringBuffer out_buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(out_buf);
    output.Accept(writer);

    const char * out_ch = out_buf.GetString();
    int out_len = out_buf.Size();

    LOG_D_A("Sending back response:%d\n%s", out_len, out_ch);

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

int64_t 
JSON_RPC_get_data_description::operator()(rapidjson::Value & params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  char * algorithm_name;
  rpc_params_t p[] = {
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (contains_params(p, params)) {
    StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
    if (!alg_state) {
      LOG_E ("AlgState is totally invalid");
      return json_rpc_error(-32603, result, allocator);
    }
    return description_string_to_json(alg_state->data_description.c_str(), result, allocator);
  } else {
    return json_rpc_error(-32602, result, allocator);
  }

}

int
description_string_to_json (const char * description_string,
  rapidjson::Value& rtn,
  rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Value a(rapidjson::kArrayType);

  /* the description string is space-delimited */
  char * ptr = strtok (tmp, " ");

  /* skip the formatting */
  char * pch = strtok (NULL, " ");

  while (pch != NULL)
  {
    rapidjson::Value v;
    v.SetString(pch, strlen(pch), allocator);
    a.PushBack(v, allocator);

    pch = strtok (NULL, " ");
  }

  rtn.AddMember("alg_data", a, allocator);

  free(tmp);
  return 0;
}


int64_t 
JSON_RPC_get_data_array_range::operator()(rapidjson::Value & params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  char * algorithm_name;
  char * data_array_name;
  int64_t count, offset;
  bool strings;
  rpc_params_t p[] = {
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {"data", TYPE_STRING, &data_array_name, false, 0},
    {"offset", TYPE_INT64, &offset, false, 0},
    {"count", TYPE_INT64, &count, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (contains_params(p, params)) {
    StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
    if (!alg_state) {
      LOG_E ("AlgState is totally invalid");
      return json_rpc_error(-32603, result, allocator);
    }
    return array_to_json_range(
	server_state->get_stinger(),
	alg_state->data_description.c_str(),
	STINGER_MAX_LVERTICES,
	(uint8_t *)alg_state->data,
	data_array_name,
	offset,
	offset+count,
	strings,
	result,
	allocator);
  } else {
    return json_rpc_error(-32602, result, allocator);
  }
}

int
array_to_json_range (stinger_t * S,
		     const char * description_string, int64_t nv, uint8_t * data,
		     const char * search_string, int64_t start, int64_t end,
		     bool strings,
		     rapidjson::Value& rtn,
		     rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  if (start >= nv) {
    LOG_E_A("Invalid range: %ld to %ld. Expecting [0, %ld).", start, end, nv);
    return json_rpc_error(-32602, rtn, allocator);
  }
  if (end > nv) {
    end = nv;
    LOG_W_A("Invalid end of range: %ld. Expecting less than %ld.", end, nv);
  }
  if (!S & strings) {
    LOG_E("STINGER pointer must be valid in order to process strings");
    return json_rpc_error(-32603, rtn, allocator);
  }

  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Value result (rapidjson::kObjectType);
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_val (rapidjson::kArrayType);
  rapidjson::Value vtx_str (rapidjson::kArrayType);

  /* the description string is space-delimited */
  char * ptr = strtok (tmp, " ");

  /* skip the formatting */
  char * pch = strtok (NULL, " ");

  int64_t done = 0;

  while (pch != NULL)
  {
    if (strcmp(pch, search_string) == 0) {
      switch (description_string[off]) {
	case 'f':
	  {
	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = start; i < end; i++) {
	      value.SetDouble((double)((float *) data)[i]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(i);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, i, &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    done = 1;
	    break;
	  }

	case 'd':
	  {
	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = start; i < end; i++) {
	      value.SetDouble((double)((double *) data)[i]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(i);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, i, &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    done = 1;
	    break;
	  }

	case 'i':
	  {
	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = start; i < end; i++) {
	      value.SetInt((int)((int32_t *) data)[i]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(i);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, i, &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    done = 1;
	    break;
	  }

	case 'l':
	  {
	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = start; i < end; i++) {
	      value.SetInt64((int64_t)((int64_t *) data)[i]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(i);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, i, &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    done = 1;
	    break;
	  }

	case 'b':
	  {
	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = start; i < end; i++) {
	      value.SetInt((int)((uint8_t *) data)[i]);
	      vtx_id.PushBack(value, allocator);
	      name.SetInt64(i);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, i, &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    done = 1;
	    break;
	  }

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }
      


    } else {
      switch (description_string[off]) {
	case 'f':
	  data += (nv * sizeof(float));
	  break;

	case 'd':
	  data += (nv * sizeof(double));
	  break;

	case 'i':
	  data += (nv * sizeof(int32_t));
	  break;

	case 'l':
	  data += (nv * sizeof(int64_t));
	  break;

	case 'b':
	  data += (nv * sizeof(uint8_t));
	  break;

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }
      off++;
    }
    
    if (done)
      break;

    pch = strtok (NULL, " ");
  }

  free(tmp);
  if (done) {
    rapidjson::Value offset, count;
    offset.SetInt64(start);
    count.SetInt64(end-start);
    result.AddMember("offset", offset, allocator);
    result.AddMember("count", count, allocator);
    result.AddMember("vertex_id", vtx_id, allocator);
    if (strings)
      result.AddMember("vertex_str", vtx_str, allocator);
    result.AddMember("value", vtx_val, allocator);

    rtn.AddMember(search_string, result, allocator);
  }
  else {
    return json_rpc_error(-32602, rtn, allocator);
  }

  return 0;
}


int64_t 
JSON_RPC_get_data_array_sorted_range::operator()(rapidjson::Value & params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  char * algorithm_name;
  char * data_array_name;
  int64_t count, offset;
  char * order;
  bool strings;
  rpc_params_t p[] = {
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {"data", TYPE_STRING, &data_array_name, false, 0},
    {"offset", TYPE_INT64, &offset, false, 0},
    {"count", TYPE_INT64, &count, false, 0},
    {"order", TYPE_STRING, &order, true, (int64_t)"DESC"},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (contains_params(p, params)) {
    StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
    if (!alg_state) {
      LOG_E ("AlgState is totally invalid");
      return json_rpc_error(-32603, result, allocator);
    }
    return array_to_json_sorted_range(
	server_state->get_stinger(),
	alg_state->data_description.c_str(),
	STINGER_MAX_LVERTICES,
	(uint8_t *)alg_state->data,
	data_array_name,
	offset,
	offset+count,
	order,
	strings,
	result,
	allocator);
  } else {
    return json_rpc_error(-32602, result, allocator);
  }
}

int
array_to_json_sorted_range (stinger_t * S,
			    const char * description_string, int64_t nv, uint8_t * data,
			    const char * search_string, int64_t start, int64_t end,
			    const char * order_str,
			    bool strings,
			    rapidjson::Value& rtn,
			    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  if (start >= nv) {
    LOG_E_A("Invalid range: %ld to %ld. Expecting [0, %ld).", start, end, nv);
    return json_rpc_error(-32602, rtn, allocator);
  }
  if (end > nv) {
    end = nv;
    LOG_W_A("Invalid end of range: %ld. Expecting less than %ld.", end, nv);
  }
  if (!S & strings) {
    LOG_E("STINGER pointer must be valid in order to process strings");
    return json_rpc_error(-32603, rtn, allocator);
  }
  
  bool asc;
  if (strncmp(order_str, "ASC", 3)==0) {
    asc = true;
  }
  else if (strncmp(order_str, "DESC", 4)==0) {
    asc = false;
  }
  else {
    return json_rpc_error(-32603, rtn, allocator);
  }

  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Value result (rapidjson::kObjectType);
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_val (rapidjson::kArrayType);
  rapidjson::Value vtx_str (rapidjson::kArrayType);

  /* the description string is space-delimited */
  char * ptr = strtok (tmp, " ");

  /* skip the formatting */
  char * pch = strtok (NULL, " ");

  int64_t done = 0;

  while (pch != NULL)
  {
    if (strcmp(pch, search_string) == 0) {
      switch (description_string[off]) {
	case 'f':
	  {
	    int64_t * idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	    for (int64_t i = 0; i < nv; i++) {
	      idx[i] = i;
	    }

	    if (asc) {
	      compare_pair_asc<float> comparator((float *) data);
	      std::sort(idx, idx + nv, comparator);
	    } else {
	      compare_pair_desc<float> comparator((float *) data);
	      std::sort(idx, idx + nv, comparator);
	    }

	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = start; i < end; i++) {
	      value.SetDouble((double) ((float *) data)[idx[i]]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(idx[i]);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, idx[i], &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    free(idx);
	    done = 1;
	    break;
	  }

	case 'd':
	  {
	    int64_t * idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	    for (int64_t i = 0; i < nv; i++) {
	      idx[i] = i;
	    }

	    if (asc) {
	      compare_pair_asc<double> comparator((double *) data);
	      std::sort(idx, idx + nv, comparator);
	    } else {
	      compare_pair_desc<double> comparator((double *) data);
	      std::sort(idx, idx + nv, comparator);
	    }

	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = start; i < end; i++) {
	      value.SetDouble((double) ((double *) data)[idx[i]]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(idx[i]);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, idx[i], &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    free(idx);
	    done = 1;
	    break;
	  }

	case 'i':
	  {
	    int64_t * idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	    for (int64_t i = 0; i < nv; i++) {
	      idx[i] = i;
	    }

	    if (asc) {
	      compare_pair_asc<int32_t> comparator((int32_t *) data);
	      std::sort(idx, idx + nv, comparator);
	    } else {
	      compare_pair_desc<int32_t> comparator((int32_t *) data);
	      std::sort(idx, idx + nv, comparator);
	    }

	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = start; i < end; i++) {
	      value.SetInt((int) ((int32_t *) data)[idx[i]]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(idx[i]);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, idx[i], &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    free(idx);
	    done = 1;
	    break;
	  }

	case 'l':
	  {
	    int64_t * idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	    for (int64_t i = 0; i < nv; i++) {
	      idx[i] = i;
	    }

	    if (asc) {
	      compare_pair_asc<int64_t> comparator((int64_t *) data);
	      std::sort(idx, idx + nv, comparator);
	    } else {
	      compare_pair_desc<int64_t> comparator((int64_t *) data);
	      std::sort(idx, idx + nv, comparator);
	    }

	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = start; i < end; i++) {
	      value.SetInt64((int64_t) ((int64_t *) data)[idx[i]]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(idx[i]);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, idx[i], &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    free(idx);
	    done = 1;
	    break;
	  }

	case 'b':
	  {
	    int64_t * idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	    for (int64_t i = 0; i < nv; i++) {
	      idx[i] = i;
	    }

	    if (asc) {
	      compare_pair_asc<uint8_t> comparator((uint8_t *) data);
	      std::sort(idx, idx + nv, comparator);
	    } else {
	      compare_pair_desc<uint8_t> comparator((uint8_t *) data);
	      std::sort(idx, idx + nv, comparator);
	    }

	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = start; i < end; i++) {
	      value.SetInt((int) ((uint8_t *) data)[idx[i]]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(idx[i]);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, idx[i], &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    free(idx);
	    done = 1;
	    break;
	  }

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }
      


    } else {
      switch (description_string[off]) {
	case 'f':
	  data += (nv * sizeof(float));
	  break;

	case 'd':
	  data += (nv * sizeof(double));
	  break;

	case 'i':
	  data += (nv * sizeof(int32_t));
	  break;

	case 'l':
	  data += (nv * sizeof(int64_t));
	  break;

	case 'b':
	  data += (nv * sizeof(uint8_t));
	  break;

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }
      off++;
    }
    
    if (done)
      break;

    pch = strtok (NULL, " ");
  }

  free(tmp);
  if (done) {
    rapidjson::Value offset, count, order;
    offset.SetInt64(start);
    count.SetInt64(end-start);
    order.SetString(order_str);
    result.AddMember("offset", offset, allocator);
    result.AddMember("count", count, allocator);
    result.AddMember("order", order, allocator);
    result.AddMember("vertex_id", vtx_id, allocator);
    if (strings)
      result.AddMember("vertex_str", vtx_str, allocator);
    result.AddMember("value", vtx_val, allocator);

    rtn.AddMember(search_string, result, allocator);
  }
  else {
    return json_rpc_error(-32602, rtn, allocator);
  }

  return 0;
}


int64_t 
JSON_RPC_get_data_array_set::operator()(rapidjson::Value & params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  char * algorithm_name;
  char * data_array_name;
  params_array_t set_array;
  bool strings;
  rpc_params_t p[] = {
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {"data", TYPE_STRING, &data_array_name, false, 0},
    {"set", TYPE_ARRAY, &set_array, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (contains_params(p, params)) {
    StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
    if (!alg_state) {
      LOG_E ("AlgState is totally invalid");
      return json_rpc_error(-32603, result, allocator);
    }
    return array_to_json_set(
	server_state->get_stinger(),
	alg_state->data_description.c_str(),
	STINGER_MAX_LVERTICES,
	(uint8_t *)alg_state->data,
	data_array_name,
	set_array.arr,
	set_array.len,
	strings,
	result,
	allocator);
  } else {
    return json_rpc_error(-32602, result, allocator);
  }
}

int
array_to_json_set (stinger_t * S,
		   const char * description_string, int64_t nv, uint8_t * data,
		   const char * search_string, int64_t * set, int64_t set_len,
		   bool strings,
		   rapidjson::Value& rtn,
		   rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  if (set_len < 1) {
    LOG_E_A("Invalid set length: %ld.", set_len);
  }
  if (!set) {
    LOG_E("Vertex set is null.");
  }
  if (!S & strings) {
    LOG_E("STINGER pointer must be valid in order to process strings");
    return json_rpc_error(-32603, rtn, allocator);
  }

  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Value result (rapidjson::kObjectType);
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_val (rapidjson::kArrayType);
  rapidjson::Value vtx_str (rapidjson::kArrayType);

  /* the description string is space-delimited */
  char * ptr = strtok (tmp, " ");

  /* skip the formatting */
  char * pch = strtok (NULL, " ");

  int64_t done = 0;

  LOG_D_A("set is %ld long", set_len);

  while (pch != NULL)
  {
    if (strcmp(pch, search_string) == 0) {
      switch (description_string[off]) {
	case 'f':
	  {
	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = 0; i < set_len; i++) {
	      int64_t vtx = set[i];
	      value.SetDouble((double)((float *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, vtx, &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    done = 1;
	    break;
	  }

	case 'd':
	  {
	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = 0; i < set_len; i++) {
	      int64_t vtx = set[i];
	      value.SetDouble((double)((double *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, vtx, &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    done = 1;
	    break;
	  }

	case 'i':
	  {
	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = 0; i < set_len; i++) {
	      int64_t vtx = set[i];
	      value.SetInt((int)((int32_t *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, vtx, &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    done = 1;
	    break;
	  }

	case 'l':
	  {
	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = 0; i < set_len; i++) {
	      int64_t vtx = set[i];
	      value.SetInt64((int64_t)((int64_t *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, vtx, &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    done = 1;
	    break;
	  }

	case 'b':
	  {
	    rapidjson::Value value, name, vtx_phys;
	    for (int64_t i = 0; i < set_len; i++) {
	      int64_t vtx = set[i];
	      value.SetInt((int)((uint8_t *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);
	      if (strings) {
		char * physID;
		uint64_t len;
		stinger_mapping_physid_direct(S, vtx, &physID, &len);
		vtx_phys.SetString(physID);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }
	    done = 1;
	    break;
	  }

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }
      


    } else {
      switch (description_string[off]) {
	case 'f':
	  data += (nv * sizeof(float));
	  break;

	case 'd':
	  data += (nv * sizeof(double));
	  break;

	case 'i':
	  data += (nv * sizeof(int32_t));
	  break;

	case 'l':
	  data += (nv * sizeof(int64_t));
	  break;

	case 'b':
	  data += (nv * sizeof(uint8_t));
	  break;

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }
      off++;
    }
    
    if (done)
      break;

    pch = strtok (NULL, " ");
  }

  free(tmp);
  if (done) {
    result.AddMember("vertex_id", vtx_id, allocator);
    if (strings)
      result.AddMember("vertex_str", vtx_str, allocator);
    result.AddMember("value", vtx_val, allocator);

    rtn.AddMember(search_string, result, allocator);
  }
  else {
    return json_rpc_error(-32602, rtn, allocator);
  }

  return 0;
}


int64_t 
JSON_RPC_get_data_array::operator()(rapidjson::Value & params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  char * algorithm_name;
  char * data_array_name;
  bool strings;
  rpc_params_t p[] = {
    {"name", TYPE_STRING, &algorithm_name, false, 0},
    {"data", TYPE_STRING, &data_array_name, false, 0},
    {"strings", TYPE_BOOL, &strings, true, 0},
    {NULL, TYPE_NONE, NULL, false, 0}
  };

  if (contains_params(p, params)) {
    StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
    if (!alg_state) {
      LOG_E ("AlgState is totally invalid");
      return json_rpc_error(-32603, result, allocator);
    }
    return array_to_json(
	server_state->get_stinger(),
	alg_state->data_description.c_str(),
	STINGER_MAX_LVERTICES,
	(uint8_t *)alg_state->data,
	data_array_name,
	strings,
	result,
	allocator);
  } else {
    return json_rpc_error(-32602, result, allocator);
  }
}

int
array_to_json (stinger_t * S,
	       const char * description_string, int64_t nv, uint8_t * data,
	       const char * search_string,
	       bool strings,
	       rapidjson::Value& val,
	       rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  return array_to_json_range (S, description_string, nv, data, search_string, 0, nv, strings, val, allocator);
}


int
main (void)
{
  JSON_RPCServerState & server_state = JSON_RPCServerState::get_server_state();
  server_state.add_rpc_function("get_data_description", new JSON_RPC_get_data_description(&server_state));
  server_state.add_rpc_function("get_data_array_range", new JSON_RPC_get_data_array_range(&server_state));
  server_state.add_rpc_function("get_data_array_sorted_range",	new JSON_RPC_get_data_array_sorted_range(&server_state));
  server_state.add_rpc_function("get_data_array", new JSON_RPC_get_data_array(&server_state));
  server_state.add_rpc_function("get_data_array_set", new JSON_RPC_get_data_array_set(&server_state));

  mon_connect(10103, "localhost", "json_rpc");

  const char * description_string = "dfill mean test data kcore neighbors";
  int64_t nv = STINGER_MAX_LVERTICES;
  size_t sz = 0;

  sz += nv * sizeof(double);
  sz += nv * sizeof(float);
  sz += nv * sizeof(int);
  sz += 2 * nv * sizeof(int64_t);

  char * data = (char *) xmalloc(sz);
  char * tmp = data;

  for (int64_t i = 0; i < nv; i++) {
    ((double *)tmp)[i] = (double) i * 0.5;
  }
  tmp += nv * sizeof(double);

  for (int64_t i = 0; i < nv; i++) {
    ((float *)tmp)[i] = 5.0 * (float) i + 0.5;
  }
  tmp += nv * sizeof(float);

  for (int64_t i = 0; i < nv; i++) {
    ((int *)tmp)[i] = (int) i;
  }
  tmp += nv * sizeof(int);

  for (int64_t i = 0; i < nv; i++) {
    ((int64_t *)tmp)[i] = 2 * (int64_t) i;
  }
  tmp += nv * sizeof(int64_t);

  for (int64_t i = 0; i < nv; i++) {
    ((int64_t *)tmp)[i] = 4 * (int64_t) i;
  }
  tmp += nv * sizeof(int64_t);

  std::map<std::string, StingerAlgState*> a;
  std::vector<StingerAlgState*> b;
  a["pagerank"] = new StingerAlgState();
  a["pagerank"]->name = "pagerank";
  a["pagerank"]->data_description = description_string;
  a["pagerank"]->data = data;
  a["pagerank"]->data_per_vertex = 32;
  b.push_back(a["pagerank"]);
  server_state.update_algs(NULL, "", 0, &b, &a);


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
  

/*
*   - f = float
*   - d = double
*   - i = int32_t
*   - l = int64_t
*   - b = byte
*
* dll mean kcore neighbors
*/

  //rapidjson::Document document;
  //document.SetObject();

  //rapidjson::Value result;
  //result.SetObject();
  //description_string_to_json (description_string, result, document.GetAllocator());
  //array_to_json (description_string, nv, (uint8_t *)data, "test", result, document.GetAllocator());
  //array_to_json_range (description_string, nv, (uint8_t *)data, "neighbors", 5, 10, result, document.GetAllocator());
  //array_to_json_sorted_range (description_string, nv, (uint8_t *)data, "data", 0, 10, "DESC", result, document.GetAllocator());

  //int64_t test_set[5] = {0, 5, 6, 7, 2};
  //array_to_json_set (description_string, nv, (uint8_t *)data, "neighbors", (int64_t *)&test_set, 5, result, document.GetAllocator());

  //rapidjson::Value id;
  //id.SetInt(64);

  //rapidjson::Document test;
  //test.SetArray();
  //json_rpc_error(document, -32100, id);
  //json_rpc_response(document, result, id);
  //json_rpc_process_request(test, document);


  //rapidjson::StringBuffer strbuf;
  //rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  //document.Accept(writer);
  //printf("--\n%s\n--\n", strbuf.GetString());

  while(1) {
  }
  free(data);
  return 0;
}

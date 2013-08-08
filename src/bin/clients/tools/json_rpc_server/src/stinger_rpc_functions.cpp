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

#include "json_rpc_server.h"
#include "json_rpc.h"

using namespace gt::stinger;


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



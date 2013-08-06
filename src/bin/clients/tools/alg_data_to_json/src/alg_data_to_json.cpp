#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <functional>

extern "C" {
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
}

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include "alg_data_to_json.h"
#include "json_rpc.h"
#include "rpc_state.h"

using namespace gt::stinger;

int64_t 
JSON_RPC_get_data_description::operator()(rapidjson::Value & params, rapidjson::Value & result, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) {
  char * algorithm_name;
  rpc_params_t p[] = {
    {"algorithm_name", TYPE_STRING, &algorithm_name},
    {NULL, TYPE_NONE, NULL}
  };

  if (contains_params(p, params)) {
    StingerAlgState * alg_state = server_state->get_alg(p->name);
    //return description_string_to_json(alg_state->data_description.c_str(), result, allocator);
  } else {
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


int
array_to_json_range (const char * description_string, int64_t nv, uint8_t * data,
		     const char * search_string, int64_t start, int64_t end,
		     rapidjson::Value& rtn,
		     rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  if (start >= nv) {
    LOG_E_A("Invalid range: %ld to %ld. Expecting [0, %ld).", start, end, nv);
  }
  if (end >= nv) {
    end = nv;
    LOG_W_A("Invalid end of range: %ld. Expecting less than %ld.", end, nv);
  }

  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Value result (rapidjson::kObjectType);
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_val (rapidjson::kArrayType);

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
	    rapidjson::Value value, name;
	    for (int64_t i = start; i < end; i++) {
	      value.SetDouble((double)((float *) data)[i]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(i);
	      vtx_id.PushBack(name, allocator);
	    }
	    done = 1;
	    break;
	  }

	case 'd':
	  {
	    rapidjson::Value value, name;
	    for (int64_t i = start; i < end; i++) {
	      value.SetDouble((double)((double *) data)[i]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(i);
	      vtx_id.PushBack(name, allocator);
	    }
	    done = 1;
	    break;
	  }

	case 'i':
	  {
	    rapidjson::Value value, name;
	    for (int64_t i = start; i < end; i++) {
	      value.SetInt((int)((int32_t *) data)[i]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(i);
	      vtx_id.PushBack(name, allocator);
	    }
	    done = 1;
	    break;
	  }

	case 'l':
	  {
	    rapidjson::Value value, name;
	    for (int64_t i = start; i < end; i++) {
	      value.SetInt64((int64_t)((int64_t *) data)[i]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(i);
	      vtx_id.PushBack(name, allocator);
	    }
	    done = 1;
	    break;
	  }

	case 'b':
	  {
	    rapidjson::Value value, name;
	    for (int64_t i = start; i < end; i++) {
	      value.SetInt((int)((uint8_t *) data)[i]);
	      vtx_id.PushBack(value, allocator);
	      name.SetInt64(i);
	      vtx_id.PushBack(name, allocator);
	    }
	    done = 1;
	    break;
	  }

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  done = 1;

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

      }
      off++;
    }
    
    if (done)
      break;

    pch = strtok (NULL, " ");
  }

  rapidjson::Value offset, count;
  offset.SetInt64(start);
  count.SetInt64(end-start);
  result.AddMember("offset", offset, allocator);
  result.AddMember("count", count, allocator);
  result.AddMember("vertex_id", vtx_id, allocator);
  result.AddMember("value", vtx_val, allocator);

  rtn.AddMember(search_string, result, allocator);

  free(tmp);
  return 0;
}


int
array_to_json_sorted_range (const char * description_string, int64_t nv, uint8_t * data,
			    const char * search_string, int64_t start, int64_t end,
			    rapidjson::Value& rtn,
			    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  if (start >= nv) {
    LOG_E_A("Invalid range: %ld to %ld. Expecting [0, %ld).", start, end, nv);
  }
  if (end >= nv) {
    end = nv;
    LOG_W_A("Invalid end of range: %ld. Expecting less than %ld.", end, nv);
  }

  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Value result (rapidjson::kObjectType);
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_val (rapidjson::kArrayType);

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

	    compare_pair_desc<float> comparator((float *) data);
	    std::sort(idx, idx + nv, comparator);

	    rapidjson::Value value, name;
	    for (int64_t i = start; i < end; i++) {
	      value.SetDouble((double) ((float *) data)[idx[i]]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(idx[i]);
	      vtx_id.PushBack(name, allocator);
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

	    compare_pair_desc<double> comparator((double *) data);
	    std::sort(idx, idx + nv, comparator);

	    rapidjson::Value value, name;
	    for (int64_t i = start; i < end; i++) {
	      value.SetDouble((double) ((double *) data)[idx[i]]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(idx[i]);
	      vtx_id.PushBack(name, allocator);
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

	    compare_pair_desc<int32_t> comparator((int32_t *) data);
	    std::sort(idx, idx + nv, comparator);

	    rapidjson::Value value, name;
	    for (int64_t i = start; i < end; i++) {
	      value.SetInt((int) ((int32_t *) data)[idx[i]]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(idx[i]);
	      vtx_id.PushBack(name, allocator);
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

	    compare_pair_desc<int64_t> comparator((int64_t *) data);
	    std::sort(idx, idx + nv, comparator);

	    rapidjson::Value value, name;
	    for (int64_t i = start; i < end; i++) {
	      value.SetInt64((int64_t) ((int64_t *) data)[idx[i]]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(idx[i]);
	      vtx_id.PushBack(name, allocator);
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

	    compare_pair_desc<uint8_t> comparator((uint8_t *) data);
	    std::sort(idx, idx + nv, comparator);

	    rapidjson::Value value, name;
	    for (int64_t i = start; i < end; i++) {
	      value.SetInt((int) ((uint8_t *) data)[idx[i]]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(idx[i]);
	      vtx_id.PushBack(name, allocator);
	    }
	    free(idx);
	    done = 1;
	    break;
	  }

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  done = 1;

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

      }
      off++;
    }
    
    if (done)
      break;

    pch = strtok (NULL, " ");
  }

  rapidjson::Value offset, count;
  offset.SetInt64(start);
  count.SetInt64(end-start);
  result.AddMember("offset", offset, allocator);
  result.AddMember("count", count, allocator);
  result.AddMember("vertex_id", vtx_id, allocator);
  result.AddMember("value", vtx_val, allocator);

  rtn.AddMember(search_string, result, allocator);

  free(tmp);
  return 0;
}


int
array_to_json_set (const char * description_string, int64_t nv, uint8_t * data,
		   const char * search_string, int64_t * set, int64_t set_len,
		   rapidjson::Value& rtn,
		   rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  if (set_len < 1) {
    LOG_E_A("Invalid set length: %ld.", set_len);
  }
  if (!set) {
    LOG_E("Vertex set is null.");
  }

  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Value result (rapidjson::kObjectType);
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_val (rapidjson::kArrayType);

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
	    rapidjson::Value value, name;
	    for (int64_t i = 0; i < set_len; i++) {
	      int64_t vtx = set[i];
	      value.SetDouble((double)((float *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);
	    }
	    done = 1;
	    break;
	  }

	case 'd':
	  {
	    rapidjson::Value value, name;
	    for (int64_t i = 0; i < set_len; i++) {
	      int64_t vtx = set[i];
	      value.SetDouble((double)((double *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);
	    }
	    done = 1;
	    break;
	  }

	case 'i':
	  {
	    rapidjson::Value value, name;
	    for (int64_t i = 0; i < set_len; i++) {
	      int64_t vtx = set[i];
	      value.SetInt((int)((int32_t *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);
	    }
	    done = 1;
	    break;
	  }

	case 'l':
	  {
	    rapidjson::Value value, name;
	    for (int64_t i = 0; i < set_len; i++) {
	      int64_t vtx = set[i];
	      value.SetInt64((int64_t)((int64_t *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);
	    }
	    done = 1;
	    break;
	  }

	case 'b':
	  {
	    rapidjson::Value value, name;
	    for (int64_t i = 0; i < set_len; i++) {
	      int64_t vtx = set[i];
	      value.SetInt((int)((uint8_t *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);
	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);
	    }
	    done = 1;
	    break;
	  }

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  done = 1;

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

      }
      off++;
    }
    
    if (done)
      break;

    pch = strtok (NULL, " ");
  }

  result.AddMember("vertex_id", vtx_id, allocator);
  result.AddMember("value", vtx_val, allocator);

  rtn.AddMember(search_string, result, allocator);

  free(tmp);
  return 0;
}


int
array_to_json (const char * description_string, int64_t nv, uint8_t * data,
	       const char * search_string,
	       rapidjson::Value& val,
	       rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
{
  return array_to_json_range (description_string, nv, data, search_string, 0, nv, val, allocator);
}


int
main (void)
{
  JSON_RPCServerState & server_state = JSON_RPCServerState::get_server_state();

  char * description_string = "dfill mean test data kcore neighbors";
  int64_t nv = 20;
  size_t sz = 0;

  sz += nv * sizeof(double);
  sz += nv * sizeof(float);
  sz += nv * sizeof(int);
  sz += 2 * nv * sizeof(int64_t);

  char * data = (char *) xmalloc(sz);
  char * tmp = data;

  for (int64_t i = 0; i < nv; i++) {
    ((double *)tmp)[i] = (double) i + 0.5;
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



/*
*   - f = float
*   - d = double
*   - i = int32_t
*   - l = int64_t
*   - b = byte
*
* dll mean kcore neighbors
*/

  rapidjson::Document document;
  document.SetObject();

  rapidjson::Value result;
  result.SetObject();
  description_string_to_json (description_string, result, document.GetAllocator());
  //array_to_json (description_string, nv, (uint8_t *)data, "test", result, document.GetAllocator());
  //array_to_json_range (description_string, nv, (uint8_t *)data, "neighbors", 5, 10, result, document.GetAllocator());
  //array_to_json_sorted_range (description_string, nv, (uint8_t *)data, "test", 0, 10, result, document.GetAllocator());

  //int64_t test_set[5] = {0, 5, 6, 7, 2};
  //array_to_json_set (description_string, nv, (uint8_t *)data, "neighbors", (int64_t *)&test_set, 5, result, document.GetAllocator());

  rapidjson::Value id;
  id.SetInt(64);

  rapidjson::Document test;
  test.SetArray();
  //json_rpc_error(document, -32100, id);
  json_rpc_response(document, result, id);
  //json_rpc_process_request(test, document);


  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  document.Accept(writer);
  printf("--\n%s\n--\n", strbuf.GetString());

  free(data);
  return 0;
}

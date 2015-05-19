#include "json_rpc_server.h"
#include "json_rpc.h"
#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"


using namespace gt::stinger;


/* Big monolithic function that does everything related to the data array */
int
array_to_json_monolithic   (json_rpc_array_meth_t method, stinger_t * S,
			    rapidjson::Value& rtn,
			    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator,
			    const char * description_string, int64_t nv, uint8_t * data,
			    bool strings,
			    const char * search_string,
			    int64_t stride,
			    bool logscale,
			    int64_t start, int64_t end,
			    const char * order_str,
			    int64_t * set, int64_t set_len
			    )
{
  if (method == SET) {
    if (set_len < 1) {
      LOG_E_A("Invalid set length: %ld.", set_len);
    }
    if (!set) {
      LOG_E("Vertex set is null.");
    }
  }
  if (method == SORTED || method == RANGE) {
    if (start >= nv) {
      LOG_E_A("Invalid range: %ld to %ld. Expecting [0, %ld).", start, end, nv);
      return json_rpc_error(-32602, rtn, allocator);
    }
    if (end > nv) {
      end = nv;
      LOG_W_A("Invalid end of range: %ld. Expecting less than %ld.", end, nv);
    }
  }
  if (!S && strings) {
    LOG_E("STINGER pointer must be valid in order to process strings");
    return json_rpc_error(-32603, rtn, allocator);
  }
  if (stride <= 0) {
    LOG_W_A("Stride of %ld is not allowed. Fixing.", stride);
    stride = 1;
  }
  if (stride >= nv) {
    LOG_W_A("Stride of %ld only returns one value. This probably isn't what you want.", stride);
  }
  
  bool asc;
  if (method == SORTED) {
    if (strncmp(order_str, "ASC", 3)==0) {
      asc = true;
    }
    else if (strncmp(order_str, "DESC", 4)==0) {
      asc = false;
    }
    else {
      return json_rpc_error(-32603, rtn, allocator);
    }
  }

  if (method == SET) {
    start = 0;
    end = set_len;
  }

  int64_t nsamples = (end - start + 1)/(stride);

  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Value result (rapidjson::kObjectType);
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_val (rapidjson::kArrayType);
  rapidjson::Value vtx_str (rapidjson::kArrayType);

  /* the description string is space-delimited */
  char * placeholder;
  char * ptr = strtok_r (tmp, " ", &placeholder);

  /* skip the formatting */
  char * pch = strtok_r (NULL, " ", &placeholder);

  int64_t done = 0;

  LOG_D_A ("%s :: %s", description_string, search_string);
  if (pch == NULL) {
    LOG_W_A ("pch is null :: %s :: %s", description_string, search_string);
  }

  while (pch != NULL)
  {
    LOG_D_A ("%s: begin while :: %s", search_string, pch);
    if (strcmp(pch, search_string) == 0) {
      LOG_D_A ("%s: matches", search_string);
      switch (description_string[off]) {
	case 'f':
	  {
	    LOG_D_A ("%s: case f", search_string);
	    int64_t * idx;
	    if (method == SORTED) {
	      idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
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
	    }

	    rapidjson::Value value, name, vtx_phys;
	    double factor = pow((double)(end - start), 1.0 /(double)nsamples);
	    for (double i = start; i < end; i += stride) {
	      if (logscale) {
		if (i != start) {
		  i -= stride;
		  int64_t prev = i;
		  if (i != start) {
		    i = pow (factor, log((double) (i - start)) / log (factor) + 1);
		  } else {
		    i = pow (factor, 1);
		  }
		  if (prev == ((int64_t) i))
		    continue;
		} 
	      }

	      int64_t vtx;
	      if (method == SORTED)
		vtx = idx[(int64_t)i];
	      if (method == RANGE)
		vtx = i;
	      if (method == SET)
		vtx = set[(int64_t)i];
	      value.SetDouble((double)((float *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);
	      
	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);

	      if (strings) {
		char * physID;
		uint64_t len;
		if(-1 == stinger_mapping_physid_direct(S, vtx, &physID, &len)) {
		  physID = (char *) "";
		  len = 0;
		}
		vtx_phys.SetString(physID, len, allocator);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }

	    if (method == SORTED)
	      free(idx);
	    done = 1;
	    break;
	  }

	case 'd':
	  {
	    LOG_D_A ("%s: case d", search_string);
	    int64_t * idx;
	    if (method == SORTED) {
	      idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
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
	    }

	    rapidjson::Value value, name, vtx_phys;
	    double factor = pow((double)(end - start), 1.0 /(double)nsamples);
	    for (double i = start; i < end; i += stride) {
	      if (logscale) {
		if (i != start) {
		  i -= stride;
		  int64_t prev = i;
		  if (i != start) {
		    i = pow (factor, log ((double) (i - start)) / log (factor) + 1);
		  } else {
		    i = pow (factor, 1);
		  }
		  if (prev == ((int64_t) i))
		    continue;
		} 
	      }

	      int64_t vtx;
	      if (method == SORTED)
		vtx = idx[(int64_t)i];
	      if (method == RANGE)
		vtx = i;
	      if (method == SET)
		vtx = set[(int64_t)i];
	      value.SetDouble((double)((double *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);

	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);

	      if (strings) {
		char * physID;
		uint64_t len;
		if(-1 == stinger_mapping_physid_direct(S, vtx, &physID, &len)) {
		  physID = (char *) "";
		  len = 0;
		}
		vtx_phys.SetString(physID, len, allocator);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }

	    if (method == SORTED)
	      free(idx);
	    done = 1;
	    break;
	  }

	case 'i':
	  {
	    LOG_D_A ("%s: case i", search_string);
	    int64_t * idx;
	    if (method == SORTED) {
	      idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
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
	    }

	    rapidjson::Value value, name, vtx_phys;
	    double factor = pow((double)(end - start), 1.0 /(double)nsamples);
	    for (double i = start; i < end; i += stride) {
	      if (logscale) {
		if (i != start) {
		  i -= stride;
		  int64_t prev = i;
		  if (i != start) {
		    i = pow (factor, log ((double) (i - start)) / log (factor) + 1);
		  } else {
		    i = pow (factor, 1);
		  }
		  if (prev == ((int64_t) i))
		    continue;
		} 
	      }

	      int64_t vtx;
	      if (method == SORTED)
		vtx = idx[(int64_t)i];
	      if (method == RANGE)
		vtx = i;
	      if (method == SET)
		vtx = set[(int64_t)i];
	      value.SetInt((int)((int32_t *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);

	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);

	      if (strings) {
		char * physID;
		uint64_t len;
		if(-1 == stinger_mapping_physid_direct(S, vtx, &physID, &len)) {
		  physID = (char *) "";
		  len = 0;
		}
		vtx_phys.SetString(physID, len, allocator);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }

	    if (method == SORTED)
	      free(idx);
	    done = 1;
	    break;
	  }

	case 'l':
	  {
	    LOG_D_A ("%s: case l", search_string);
	    int64_t * idx;
	    if (method == SORTED) {
	      idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
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
	    }

	    rapidjson::Value value, name, vtx_phys;
	    double factor = pow((double)(end - start), 1.0 /(double)nsamples);
	    for (double i = start; i < end; i += stride) {
	      if (logscale) {
		if (i != start) {
		  i -= stride;
		  int64_t prev = i;
		  if (i != start) {
		    i = pow (factor, log ((double) (i - start)) / log (factor) + 1);
		  } else {
		    i = pow (factor, 1);
		  }
		  if (prev == ((int64_t) i))
		    continue;
		} 
	      }

	      int64_t vtx;
	      if (method == SORTED)
		vtx = idx[(int64_t)i];
	      if (method == RANGE)
		vtx = i;
	      if (method == SET)
		vtx = set[(int64_t)i];
	      value.SetInt64((int64_t)((int64_t *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);

	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);

	      if (strings) {
		char * physID;
		uint64_t len;
		if(-1 == stinger_mapping_physid_direct(S, vtx, &physID, &len)) {
		  physID = (char *) "";
		  len = 0;
		}
		vtx_phys.SetString(physID, len, allocator);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }

	    if (method == SORTED)
	      free(idx);
	    done = 1;
	    break;
	  }

	case 'b':
	  {
	    LOG_D_A ("%s: case b", search_string);
	    int64_t * idx;
	    if (method == SORTED) {
	      idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
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
	    }

	    rapidjson::Value value, name, vtx_phys;
	    double factor = pow((double)(end - start), 1.0 /(double)nsamples);
	    for (double i = start; i < end; i += stride) {
	      if (logscale) {
		if (i != start) {
		  i -= stride;
		  int64_t prev = i;
		  if(i != start) {
		    i = pow (factor, log ((double) (i - start)) / log (factor) + 1);
		  } else {
		    i = pow (factor, 1);
		  }
		  if (prev == ((int64_t) i))
		    continue;
		} 
	      }

	      int64_t vtx;
	      if (method == SORTED)
		vtx = idx[(int64_t)i];
	      if (method == RANGE)
		vtx = i;
	      if (method == SET)
		vtx = set[(int64_t)i];
	      value.SetInt((int)((uint8_t *) data)[vtx]);
	      vtx_val.PushBack(value, allocator);

	      name.SetInt64(vtx);
	      vtx_id.PushBack(name, allocator);

	      if (strings) {
		char * physID;
		uint64_t len;
		if(-1 == stinger_mapping_physid_direct(S, vtx, &physID, &len)) {
		  physID = (char *) "";
		  len = 0;
		}
		vtx_phys.SetString(physID, len, allocator);
		vtx_str.PushBack(vtx_phys, allocator);
	      }
	    }

	    if (method == SORTED)
	      free(idx);
	    done = 1;
	    break;
	  }

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }
      


    } else {
      LOG_D_A ("%s: does not match %d", search_string, nv);
      switch (description_string[off]) {
	case 'f':
	  data += (S->max_nv * sizeof(float));
	  break;

	case 'd':
	  data += (S->max_nv * sizeof(double));
	  break;

	case 'i':
	  data += (S->max_nv * sizeof(int32_t));
	  break;

	case 'l':
	  data += (S->max_nv * sizeof(int64_t));
	  break;

	case 'b':
	  data += (S->max_nv * sizeof(uint8_t));
	  break;

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }
      off++;
    }

    LOG_D_A ("%s: done: %d", search_string, done);
    
    if (done)
      break;

    pch = strtok_r (NULL, " ", &placeholder);
  }

  free(tmp);
  if (done) {
    rapidjson::Value offset, count, order;
    if (method == SORTED || method == RANGE) {
      offset.SetInt64(start);
      result.AddMember("offset", offset, allocator);
      count.SetInt64(end-start);
      result.AddMember("count", count, allocator);
    }
    if (method == SORTED) {
      order.SetString(order_str, strlen(order_str), allocator);
      result.AddMember("order", order, allocator);
    }
    result.AddMember("vertex_id", vtx_id, allocator);
    if (strings)
      result.AddMember("vertex_str", vtx_str, allocator);
    result.AddMember("value", vtx_val, allocator);

    rtn.AddMember(search_string, result, allocator);
  }
  else {
    LOG_W_A ("%s: shouldn't get here", search_string);
    return json_rpc_error(-32602, rtn, allocator);
  }

  return 0;
}


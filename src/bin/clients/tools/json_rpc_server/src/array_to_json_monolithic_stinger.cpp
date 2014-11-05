//#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"
#include "json_rpc_server.h"
#include "json_rpc.h"

using namespace gt::stinger;


/* Function with the same interface as the other monotlithic, but created to supply 
 * data from stinger */
int
array_to_json_monolithic_stinger   (json_rpc_array_meth_t method, stinger_t * S,
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

  rapidjson::Value result (rapidjson::kObjectType);
  rapidjson::Value vtx_id (rapidjson::kArrayType);
  rapidjson::Value vtx_val (rapidjson::kArrayType);
  rapidjson::Value vtx_str (rapidjson::kArrayType);

  int64_t done = 0;

  LOG_D_A ("%s", search_string);

  MAP_STING(S);

  if(0 == strcmp(search_string, "vertex_outdegree")) {
      int64_t * idx;
      if (method == SORTED) {
	idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	for (int64_t i = 0; i < nv; i++) {
	  idx[i] = i;
	}

	if (asc) {
	  compare_pair_off_asc<int64_t> comparator(&(vertices->vertices[0].outDegree), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	} else {
	  compare_pair_off_desc<int64_t> comparator(&(vertices->vertices[0].outDegree), sizeof(vertices->vertices[0]));
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
	value.SetInt64(stinger_outdegree(S, vtx));
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
  } else if(0 == strcmp(search_string, "vertex_indegree")) {
      int64_t * idx;
      if (method == SORTED) {
	idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	for (int64_t i = 0; i < nv; i++) {
	  idx[i] = i;
	}

	if (asc) {
	  compare_pair_off_asc<int64_t> comparator(&(vertices->vertices[0].inDegree), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	} else {
	  compare_pair_off_desc<int64_t> comparator(&(vertices->vertices[0].inDegree), sizeof(vertices->vertices[0]));
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
	value.SetInt64(stinger_indegree(S, vtx));
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
  } else if(0 == strcmp(search_string, "vertex_weight")) {
      int64_t * idx;
      if (method == SORTED) {
	idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	for (int64_t i = 0; i < nv; i++) {
	  idx[i] = i;
	}

	if (asc) {
	  compare_pair_off_asc<int64_t> comparator(&(vertices->vertices[0].weight), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	} else {
	  compare_pair_off_desc<int64_t> comparator(&(vertices->vertices[0].weight), sizeof(vertices->vertices[0]));
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
	value.SetInt64(stinger_vweight_get(S, vtx));
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
  } else if(0 == strcmp(search_string, "vertex_type_num")) {
      int64_t * idx;
      if (method == SORTED) {
	idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	for (int64_t i = 0; i < nv; i++) {
	  idx[i] = i;
	}

	if (asc) {
	  compare_pair_off_asc<int64_t> comparator(&(vertices->vertices[0].type), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	} else {
	  compare_pair_off_desc<int64_t> comparator(&(vertices->vertices[0].type), sizeof(vertices->vertices[0]));
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
	value.SetInt64(stinger_vtype_get(S, vtx));
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
  } else if(0 == strcmp(search_string, "vertex_type_name")) {
      int64_t * idx;
      if (method == SORTED) {
	idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
	for (int64_t i = 0; i < nv; i++) {
	  idx[i] = i;
	}

	if (asc) {
	  compare_pair_off_asc<int64_t> comparator(&(vertices->vertices[0].type), sizeof(vertices->vertices[0]));
	  std::sort(idx, idx + nv, comparator);
	} else {
	  compare_pair_off_desc<int64_t> comparator(&(vertices->vertices[0].type), sizeof(vertices->vertices[0]));
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
	char * type_name_str = stinger_vtype_names_lookup_name(S,stinger_vtype_get(S, vtx));
	value.SetString(type_name_str, strlen(type_name_str), allocator);
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
  } else { 
  }


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


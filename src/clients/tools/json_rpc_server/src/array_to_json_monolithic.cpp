#include "json_rpc_server.h"
#include "json_rpc.h"
#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"


using namespace gt::stinger;

template <typename T>
inline int64_t* new_sorted_data_idx(T* data, int64_t nv, bool asc) {
  int64_t * idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
  for (int64_t i = 0; i < nv; i++) {
    idx[i] = i;
  }

  if (asc) {
    compare_pair_asc<T> comparator(data);
    std::sort(idx, idx + nv, comparator);
  } else {
    compare_pair_desc<T> comparator(data);
    std::sort(idx, idx + nv, comparator);
  }

  return idx;
}

template <typename T>
inline int64_t* new_sorted_data_idx(T* data_start, int64_t nv, bool asc, int64_t data_byte_offset) {
  int64_t * idx = (int64_t *) xmalloc (nv * sizeof(int64_t));
  for (int64_t i = 0; i < nv; i++) {
    idx[i] = i;
  }

  if (asc) {
    compare_pair_off_asc<T> comparator(data_start, data_byte_offset);
    std::sort(idx, idx + nv, comparator);
  } else {
    compare_pair_off_desc<T> comparator(data_start, data_byte_offset);
    std::sort(idx, idx + nv, comparator);
  }

  return idx;
}

inline void free_sorted_data_idx(int64_t* idx) {
  free(idx);
}


/*! \brief Converts a STINGER algorithm data array into JSON
 *
 * Takes the data contained in a STINGER algorithm and adds any data
 * in the array matching the search_string to the JSON object passed in
 *
 * An algorithm's data store is defined by description_string.  This has the format of
 * formats field1_name field2_name field3_name ...
 *
 * formats is a series of characters f, d, i, l, b that denote the field type
 * and the field_names are the names of the fields corresponding to that fields relative
 * order in the formats description
 *
 * @param method The Method to apply.  Valid options are SET, SORTED, RANGE
 * @param S The pointer to Stinger
 * @param rtn The rapidjson object where the output should be added
 * @param allocator rapidjson allocator for adding new objects to rtn
 * @param description_string String describing the algorithms data fields and types
 * @param nv Number of vertices
 * @param data The algorithm data array
 * @param strings Should vertex string names be returned in the object (requires STINGER pointer to be valid)
 * @param search_string The field name to extract from the data
 * @param vtypes The vertex types to return.  Pass NULL for all vertex types
 * @param num_vtypes The number of vertex types in the vtypes array
 * @param stride Return every 'stride' number of elements in the data array
 * @param logscale Perform strided sampling on a log scale
 * @param start Skip to this start point in the data array
 * @param end Stop at this point in the data array
 * @param order_str When SORTED is the method, what direction to sort (valid values are ASC, DESC)
 * @param set Only return values for specified vertices. Used with method SET
 * @param set_len Length of set of vertices
 * @return 0 if successful.  Returns a json_rpc_error object when unsuccessful
 */
int
array_to_json_monolithic   (json_rpc_array_meth_t method, stinger_t * S,
                            rapidjson::Value& rtn,
                            rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator,
                            const char * description_string, int64_t nv, uint8_t * data,
                            bool strings,
                            const char * search_string,
                            int64_t * vtypes, int64_t num_vtypes,
                            int64_t stride,
                            bool logscale,
                            int64_t start, int64_t end,
                            const char * order_str,
                            int64_t * set, int64_t set_len
                            )
{
  /* Bounds checking */
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
      LOG_W_A("Invalid end of range: %ld. Expecting less than %ld.", end, nv);
      end = nv;
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
  if (vtypes != NULL && num_vtypes <= 0) {
    LOG_W("Number of vertex types specified as <= 0. Assuming all vertex types");
    num_vtypes = 0;
  }

  MAP_STING(S);
  
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
      int64_t * idx;
      if (method == SORTED) {
        switch (description_string[off]) {
          case 'f':
            LOG_D_A ("%s: case f", search_string);
            idx = new_sorted_data_idx((float*)data, nv, asc);
            break;
          case 'd':
            LOG_D_A ("%s: case d", search_string);
            idx = new_sorted_data_idx((double*)data, nv, asc);
            break;
          case 'i':
            LOG_D_A ("%s: case i", search_string);
            idx = new_sorted_data_idx((int32_t*)data, nv, asc);
            break;
          case 'l':
            LOG_D_A ("%s: case l", search_string);
            idx = new_sorted_data_idx((int64_t*)data, nv, asc);
            break;
          case 'b':
            LOG_D_A ("%s: case b", search_string);
            idx = new_sorted_data_idx((uint8_t*)data, nv, asc);
            break;
          case 's':
            LOG_D_A ("%s: case s", search_string);
            idx = new_sorted_data_idx((int64_t*)stinger_local_state_get_data_ptr(S, search_string), nv, asc, sizeof(vertices->vertices[0]));
            break;
          default:
            LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
            return json_rpc_error(-32603, rtn, allocator);
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

        if (vtypes != NULL && num_vtypes > 0) {
          int64_t type = stinger_vtype_get(S, vtx);
          bool found = false;
          for (int64_t j = 0; j < num_vtypes; j++) {
            if (vtypes[j] == type) {
              found = true;
              break;
            }
          }
          if (found == false) {
            continue;
          }
        }

        switch (description_string[off]) {
          case 'f':
            value.SetDouble((double)((float *) data)[vtx]);
            break;
          case 'd':
            value.SetDouble((double)((double *) data)[vtx]);
            break;
          case 'i':
            value.SetInt((int)((int32_t *) data)[vtx]);
            break;
          case 'l':
            value.SetInt64((int64_t)((int64_t *) data)[vtx]);
            break;
          case 'b':
            value.SetInt64((int)((uint8_t *) data)[vtx]);
            break;
          case 's':
            if (0 == strncmp(search_string, "vertex_type_name",16)) {
              char * type_name_str = stinger_vtype_names_lookup_name(S, stinger_local_state_get_data(S, search_string, vtx));
              value.SetString(type_name_str, strlen(type_name_str), allocator);
            } else {
              value.SetInt64(stinger_local_state_get_data(S, search_string, vtx));
            }
            break;
          default:
            LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
            return json_rpc_error(-32603, rtn, allocator);
        }
        
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
        free_sorted_data_idx(idx);
      done = 1;
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
        case 's':
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


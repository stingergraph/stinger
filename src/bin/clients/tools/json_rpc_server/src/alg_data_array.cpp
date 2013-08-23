
#include "stinger_core/stinger_error.h"
#include "alg_data_array.h"

namespace gt {
  namespace stinger {

    AlgDataArray::AlgDataArray(
		    JSON_RPCServerState * server_state,
		    const char * algorithm_name,
		    void * data, char type, int64_t length) : t(type), d(data), len(length), state(server_state)
    { 
      strncpy(alg, algorithm_name, 128);
      StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
      if (!alg_state) {
	LOG_E ("StingerAlgState is invalid");
	return;
      }
      offset = ((uint8_t *)data) - ((uint8_t *)alg_state->data);
    }

    int64_t
    AlgDataArray::length()
    {
      return len;
    }

    char
    AlgDataArray::type()
    {
      return t;
    }

    void
    AlgDataArray::refresh()
    {
      StingerAlgState * alg_state = state->get_alg(alg);
      d = (void *)(((uint8_t *)alg_state->data) + offset);
    }

    int32_t
    AlgDataArray::get_int32(int64_t index)
    {
      if(t == 'i') {
	return ((int32_t *)d)[index];
      } else {
	LOG_W("This is not a 32-bit integer array");
	return 0;
      }
    }

    int64_t
    AlgDataArray::get_int64(int64_t index)
    {
      if(t == 'l') {
	return ((int64_t *)d)[index];
      } else {
	LOG_W("This is not a 64-bit integer array");
	return 0;
      }
    }

    float
    AlgDataArray::get_float(int64_t index)
    {
      if(t == 'f') {
	return ((float *)d)[index];
      } else {
	LOG_W("This is not a float array");
	return 0;
      }
    }

    double
    AlgDataArray::get_double(int64_t index)
    {
      if(t == 'd') {
	return ((double *)d)[index];
      } else {
	LOG_W("This is not a double array");
	return 0;
      }
    }

    uint8_t
    AlgDataArray::get_uint8(int64_t index)
    {
      if(t == 'b') {
	return ((uint8_t *)d)[index];
      } else {
	LOG_W("This is not a uint8_t array");
	return 0;
      }
    }

    bool
    AlgDataArray::equal(int64_t a, int64_t b)
    {
      switch(t) {
	case 'i':
	return ((int32_t *)d)[a] == ((int32_t *)d)[b];
	break;
	case 'l':
	return ((int64_t *)d)[a] == ((int64_t *)d)[b];
	break;
	case 'f':
	return ((float *)d)[a] == ((float *)d)[b];
	break;
	case 'd':
	return ((double *)d)[a] == ((double *)d)[b];
	break;
	case 'b':
	return ((int8_t *)d)[a] == ((int8_t *)d)[b];
	break;
	default:
	LOG_W("Array has non-standard type!");
	return false;
      }
    }

    void
    AlgDataArray::to_value(int64_t index, rapidjson::Value & val, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
    {
    }

  }
}

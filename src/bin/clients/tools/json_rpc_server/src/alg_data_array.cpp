#include "alg_data_array.h"

namespace gt {
  namespace stinger {

    AlgDataArray::AlgDataArray(StingerAlgState * alg_state, void * data, char type, int64_t length) : t(type), d(data), len(length), state(alg_state)
    { 
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

    int32_t
    AlgDataArray::get_int32(int64_t index)
    {
      d = (void *)(((uint8_t *)state->data) + offset);
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
      d = (void *)(((uint8_t *)state->data) + offset);
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
      d = (void *)(((uint8_t *)state->data) + offset);
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
      d = (void *)(((uint8_t *)state->data) + offset);
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
      d = (void *)(((uint8_t *)state->data) + offset);
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
      d = (void *)(((uint8_t *)state->data) + offset);
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
      d = (void *)(((uint8_t *)state->data) + offset);
    }

  }
}

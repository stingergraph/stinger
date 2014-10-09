
#include "stinger_core/stinger_error.h"
#include "stinger_core/xmalloc.h"
#include "alg_data_array.h"

namespace {
  template <typename T> struct tc_info { static const char tc; static const char * tname; };
#define DEF_TC_INFO(T,TC,TN)                            \
  template struct tc_info<T>;                           \
  template<> const char tc_info<T>::tc = TC;            \
  template<> const char * tc_info<T>::tname = TN

  DEF_TC_INFO(int32_t, 'i', "32-bit integer");
  DEF_TC_INFO(int64_t, 'l', "64-bit integer");
  DEF_TC_INFO(uint8_t, 'u', "8-bit unsigned integer");
  DEF_TC_INFO(float, 'f', "32-bit float");
  DEF_TC_INFO(double, 'd', "64-bit float");
}

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

    AlgDataArray::AlgDataArray(
		    JSON_RPCServerState * server_state,
		    const char * algorithm_name,
		    const char * field) : state(server_state)
    { 
      len = server_state->get_stinger()->max_nv;
      strncpy(alg, algorithm_name, 128);
      StingerAlgState * alg_state = server_state->get_alg(algorithm_name);
      if (!alg_state) {
	LOG_E ("StingerAlgState is invalid");
	return;
      }

      /* Temporary storage for description string to parse */
      char * tmp = (char *) xmalloc ((alg_state->data_description.length()+10) * sizeof(char));
      strcpy(tmp, alg_state->data_description.c_str());

      /* get a fresh copy of the alg data pointer */
      uint8_t * data = (uint8_t *) alg_state->data;


      LOG_D_A("TMP is %s", tmp);

      /* the description string is space-delimited */
      char * placeholder = NULL;
      char * ptr = strtok_r (tmp, " ", &placeholder);

      /* skip the formatting */
      char * pch = strtok_r (NULL, " ", &placeholder);

      int64_t off = 0;

      LOG_D_A("Field is %s pch is %s ptr is %s and desc is %s", field, pch, ptr, alg_state->data_description.c_str());
      while (0 != strcmp(field, pch)) {
	switch (ptr[off]) {
	  case 'f':
	    data += (len * sizeof(float));
	    break;

	  case 'd':
	    data += (len * sizeof(double));
	    break;

	  case 'i':
	    data += (len * sizeof(int32_t));
	    break;

	  case 'l':
	    data += (len * sizeof(int64_t));
	    break;

	  case 'b':
	    data += (len * sizeof(uint8_t));
	    break;

	  default:
	    LOG_W_A("Umm...what letter was that?\ndescription_string: %s", alg_state->data_description.c_str());
	    return;
	}
	off++;

	pch = strtok_r (NULL, " ", &placeholder);
	LOG_D_A("Field is %s pch is %s", field, pch);
      } /* pch */

      offset = ((uint8_t *)data) - ((uint8_t *)alg_state->data);
      t = ptr[off];
      d = data;
      free(tmp);
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

    template <typename T>
    T AlgDataArray::getval(int64_t index)
    {
      if (t == tc_info<T>::tc)
        return static_cast<T*>(d)[index];
      else {
        LOG_W_A("This is not a %s array", tc_info<T>::tname);
        return static_cast<T>(0);
      }
    }

    template int32_t AlgDataArray::getval<int32_t>(int64_t);
    template int64_t AlgDataArray::getval<int64_t>(int64_t);
    template float AlgDataArray::getval<float>(int64_t);
    template double AlgDataArray::getval<double>(int64_t);
    template uint8_t AlgDataArray::getval<uint8_t>(int64_t);

    template <typename T>
    const T* AlgDataArray::getptr()
    {
      if (t == tc_info<T>::tc)
        return static_cast<T*>(d);
      else {
        LOG_W_A("This is not a %s array", tc_info<T>::tname);
        return static_cast<T*>(NULL);
      }
    }

    template const int32_t * AlgDataArray::getptr<int32_t>();
    template const int64_t * AlgDataArray::getptr<int64_t>();
    template const float * AlgDataArray::getptr<float>();
    template const double * AlgDataArray::getptr<double>();
    template const uint8_t * AlgDataArray::getptr<uint8_t>();

    int32_t
    AlgDataArray::get_int32(int64_t index)
    {
      return getval<int32_t>(index);
    }

    int64_t
    AlgDataArray::get_int64(int64_t index)
    {
      return getval<int64_t>(index);
    }

    float
    AlgDataArray::get_float(int64_t index)
    {
      return getval<float>(index);
    }

    double
    AlgDataArray::get_double(int64_t index)
    {
      return getval<double>(index);
    }

    uint8_t
    AlgDataArray::get_uint8(int64_t index)
    {
      return getval<uint8_t>(index);
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
	LOG_W_A("Array has non-standard type [%c]", t);
	return false;
      }
    }

    void
    AlgDataArray::to_value(int64_t index, rapidjson::Value & val, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator)
    {
    }

  }
}

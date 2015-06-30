#include "json_rpc_server.h"
#include "json_rpc.h"
#include "stinger_core/xmalloc.h"
#include "rapidjson/document.h"

#define LOG_AT_W  /* warning only */
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;



int
array_to_json_reduction    (stinger_t * S,
			    rapidjson::Value& rtn,
			    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator,
			    const char * description_string, uint8_t * data,
			    const char * algorithm_name
			    )
{
  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Value fields (rapidjson::kArrayType);

  /* the description string is space-delimited */
  char * placeholder;
  char * ptr = strtok_r (tmp, " ", &placeholder);

  /* skip the formatting */
  char * pch = strtok_r (NULL, " ", &placeholder);

  if (pch == NULL) {
    LOG_W_A ("pch is null :: %s :: %s", description_string, algorithm_name);
  }

  int64_t start = 0;
  int64_t end = S->max_nv;

  while (pch != NULL)
  {
      size_t pch_len = strlen(pch);
      switch (description_string[off]) {
	case 'f':
	  {
	    rapidjson::Value reduction (rapidjson::kObjectType);
	    rapidjson::Value reduction_name, reduction_value;
	    double sum = 0.0;
	    for (int64_t i = start; i < end; i++) {
	      double val = (double)((float *) data)[i];
	      if (val > 0.0)
		sum += val;
	    }
	    reduction_value.SetDouble(sum);
	    reduction_name.SetString(pch, pch_len, allocator);
	    
	    reduction.AddMember("field", reduction_name, allocator);
	    reduction.AddMember("value", reduction_value, allocator);

	    fields.PushBack(reduction, allocator);
	    break;
	  }

	case 'd':
	  {
	    rapidjson::Value reduction (rapidjson::kObjectType);
	    rapidjson::Value reduction_name, reduction_value;
	    double sum = 0.0;
	    for (int64_t i = start; i < end; i++) {
	      double val = (double)((double *) data)[i];
	      if (val > 0.0)
		sum += val;
	    }
	    reduction_value.SetDouble(sum);
	    reduction_name.SetString(pch, pch_len, allocator);
	    
	    reduction.AddMember("field", reduction_name, allocator);
	    reduction.AddMember("value", reduction_value, allocator);

	    fields.PushBack(reduction, allocator);
	    break;
	  }

	case 'i':
	  {
	    rapidjson::Value reduction (rapidjson::kObjectType);
	    rapidjson::Value reduction_name, reduction_value;
	    int64_t sum = 0;
	    for (int64_t i = start; i < end; i++) {
	      int64_t val = (int64_t)((int32_t *) data)[i];
	      if (val > 0)
		sum += val;
	    }
	    reduction_value.SetInt64(sum);
	    reduction_name.SetString(pch, pch_len, allocator);
	    
	    reduction.AddMember("field", reduction_name, allocator);
	    reduction.AddMember("value", reduction_value, allocator);

	    fields.PushBack(reduction, allocator);
	    break;
	  }

	case 'l':
	  {
	    rapidjson::Value reduction (rapidjson::kObjectType);
	    rapidjson::Value reduction_name, reduction_value;
	    int64_t sum = 0;
	    for (int64_t i = start; i < end; i++) {
	      int64_t val = (int64_t)((int64_t *) data)[i];
	      if (val > 0)
		sum += val;
	    }
	    
	    reduction_value.SetInt64(sum);
	    reduction_name.SetString(pch, pch_len, allocator);
	    
	    reduction.AddMember("field", reduction_name, allocator);
	    reduction.AddMember("value", reduction_value, allocator);

	    fields.PushBack(reduction, allocator);
	    break;
	  }

	case 'b':
	  {
	    rapidjson::Value reduction (rapidjson::kObjectType);
	    rapidjson::Value reduction_name, reduction_value;
	    int64_t sum = 0;
	    for (int64_t i = start; i < end; i++) {
	      int64_t val = (int64_t)((uint8_t *) data)[i];
	      if (val > 0)
		sum += val;
	    }

	    reduction_value.SetInt64(sum);
	    reduction_name.SetString(pch, pch_len, allocator);
	    
	    reduction.AddMember("field", reduction_name, allocator);
	    reduction.AddMember("value", reduction_value, allocator);
	    
	    fields.PushBack(reduction, allocator);
	    break;
	  }

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return json_rpc_error(-32603, rtn, allocator);

      }

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
    
    pch = strtok_r (NULL, " ", &placeholder);
  }

  free(tmp);
  rtn.AddMember(algorithm_name, fields, allocator);

  return 0;
}

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
}

#include "rapidjson/document.h"
#include "rapidjson/filestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "alg_data_to_json.h"

rapidjson::Document *
description_string_to_json (char * description_string)
{
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Document * document = new rapidjson::Document();
  rapidjson::Document::AllocatorType& allocator = document->GetAllocator();

  document->SetObject();

  rapidjson::Value a(rapidjson::kArrayType);

  /* the description string is space-delimited */
  char * ptr = strtok (tmp, " ");

  /* skip the formatting */
  char * pch = strtok (NULL, " ");

  while (pch != NULL)
  {
    printf("pch = %s\n", pch);
    rapidjson::Value v;
    v.SetString(pch, strlen(pch), allocator);
    a.PushBack(v, allocator);

    pch = strtok (NULL, " ");
  }

  document->AddMember("alg_data", a, allocator);

  free(tmp);
  return document;
}


rapidjson::Document *
array_to_json (char * description_string, int64_t nv, uint8_t * data, char * search_string)
{
  size_t off = 0;
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));
  strcpy(tmp, description_string);

  rapidjson::Document * document = new rapidjson::Document();
  rapidjson::Document::AllocatorType& allocator = document->GetAllocator();

  document->SetObject();

  rapidjson::Value a(rapidjson::kArrayType);

  /* the description string is space-delimited */
  char * ptr = strtok (tmp, " ");

  /* skip the formatting */
  char * pch = strtok (NULL, " ");

  int64_t done = 0;

  while (pch != NULL)
  {
    printf("pch = %s\n", pch);
    if (strcmp(pch, search_string) == 0) {
      switch (description_string[off]) {
	case 'f':
	  for (int64_t i = 0; i < nv; i++) {
	    rapidjson::Value v;
	    v.SetDouble((double)((float *) data)[i]);
	    a.PushBack(v, allocator);
	  }
	  done = 1;
	  break;

	case 'd':
	  for (int64_t i = 0; i < nv; i++) {
	    rapidjson::Value v;
	    v.SetDouble((double)((double *) data)[i]);
	    a.PushBack(v, allocator);
	  }
	  done = 1;
	  break;

	case 'i':
	  for (int64_t i = 0; i < nv; i++) {
	    rapidjson::Value v;
	    v.SetInt(((int32_t *) data)[i]);
	    a.PushBack(v, allocator);
	  }
	  done = 1;
	  break;

	case 'l':
	  for (int64_t i = 0; i < nv; i++) {
	    rapidjson::Value v;
	    v.SetInt64(((int64_t *) data)[i]);
	    a.PushBack(v, allocator);
	  }
	  done = 1;
	  break;

	case 'b':
	  for (int64_t i = 0; i < nv; i++) {
	    rapidjson::Value v;
	    v.SetInt((int)((char *) data)[i]);
	    a.PushBack(v, allocator);
	  }
	  done = 1;
	  break;

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

  document->AddMember(search_string, a, allocator);

  free(tmp);
  return document;
}


int
main (void)
{
  char * description_string = "dfill mean test data kcore neighbors";
  int64_t nv = 1024;
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

  //rapidjson::Document * json = description_string_to_json (description_string);
  rapidjson::Document * json = array_to_json (description_string, nv, (uint8_t *)data, "neighbors");

  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  json->Accept(writer);
  printf("--\n%s\n--\n", strbuf.GetString());

  return 0;
}

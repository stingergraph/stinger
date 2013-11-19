#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_core/stinger.h"
#include "stinger_net/mon_handling.h"
#include "stinger_net/stinger_mon.h"

#define MONGO_HAVE_STDINT
#include "mongo_c_driver/mongo.h"
#include "alg_to_mongo.h"

using namespace gt::stinger;


int
array_to_bson   (
			    bson ** documents,
			    stinger_t * S,
			    int64_t nv,
			    StingerAlgState * alg_state,
			    uint64_t timestamp,
			    int64_t start,
			    int64_t end
			    )
{
  if (!S) {
    LOG_E("STINGER pointer is invalid");
    return -1;
  }

  if (!alg_state) {
    LOG_E("StingerAlgState pointer is invalid");
    return -1;
  }

  const char * description_string = alg_state->data_description.c_str();

  /* Temporary storage for description string to parse */
  size_t len = strlen(description_string);
  char * tmp = (char *) xmalloc ((len+1) * sizeof(char));


  int64_t item_count = 0;

  for (int64_t vtx = start; vtx < end && vtx < nv; vtx++, item_count++)
  {
    size_t off = 0;
    bson * bson_ptr = documents[item_count];
    char * physID;
    size_t len;

    /* initialize BSON object for this vertex */
    bson_init (bson_ptr);
    bson_append_new_oid (bson_ptr, "_id");
    bson_append_long (bson_ptr, "time", timestamp);

    strcpy(tmp, description_string);
    
    /* get the physical name of the vertex */
    if (-1 == stinger_mapping_physid_direct(S, vtx, &physID, &len)) {  // XXX: check and see if this is null-terminated
      physID = (char *) "\0";
      len = 0;
    }

    bson_append_string (bson_ptr, "vtx", physID);


    /* get a fresh copy of the alg data pointer */
    uint8_t * data = (uint8_t *) alg_state->data;

    /* the description string is space-delimited */
    char * placeholder;
    char * ptr = strtok_r (tmp, " ", &placeholder);

    /* skip the formatting */
    char * pch = strtok_r (NULL, " ", &placeholder);

    while (pch != NULL)
    {
      switch (description_string[off]) {
	case 'f':
	  {
	    double value = (double)((float *) data)[vtx];
	    bson_append_double (bson_ptr, pch, value);
	    break;
	  }

	case 'd':
	  {
	    double value = (double)((double *) data)[vtx];
	    bson_append_double (bson_ptr, pch, value);
	    break;
	  }

	case 'i':
	  {
	    int value = (int)((int32_t *) data)[vtx];
	    bson_append_int (bson_ptr, pch, value);
	    break;
	  }

	case 'l':
	  {
	    int64_t value = (int64_t)((int64_t *) data)[vtx];
	    bson_append_long (bson_ptr, pch, value);
	    break;
	  }

	case 'b':
	  {
	    uint8_t value = (uint8_t)((uint8_t *) data)[vtx];
	    bson_append_int (bson_ptr, pch, value);
	    break;
	  }

	default:
	  LOG_W_A("Umm...what letter was that?\ndescription_string: %s", description_string);
	  return -1;

      }

      /* advance the data pointer to the next field */
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
	  return -1;

      }
      off++;

      pch = strtok_r (NULL, " ", &placeholder);
    } /* pch */

    bson_finish (bson_ptr);

  } /* vtx */

  free(tmp);

  return item_count;
}

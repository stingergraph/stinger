#ifndef _ALG_TO_MONGO_H_
#define _ALG_TO_MONGO_H_

#include "stinger_core/stinger.h"
#include "stinger_net/mon_handling.h"
#include "stinger_net/stinger_mon.h"

#define MONGO_HAVE_STDINT
#include "mongo_c_driver/mongo.h"

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
			    );

int64_t
get_current_timestamp (void);


#endif /* _ALG_TO_MONGO_H_ */

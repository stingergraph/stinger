#ifndef _HUMAN_EDGE_GENERATOR_H
#define _HUMAN_EDGE_GENERATOR_H

#include "stinger_net/proto/stinger-batch.pb.h"
#include "stinger_net/send_rcv.h"

#undef LOG_AT_F
#undef LOG_AT_E
#undef LOG_AT_W
#undef LOG_AT_I
#undef LOG_AT_V
#undef LOG_AT_D

#define LOG_AT_D
#include "stinger_core/stinger_error.h"

using namespace gt::stinger;


int64_t
get_current_timestamp(void);

#endif /* _HUMAN_EDGE_GENERATOR_H */

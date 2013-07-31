#ifndef _RMAT_EDGE_GENERATOR_H
#define _RMAT_EDGE_GENERATOR_H

#include "stinger_net/proto/stinger-batch.pb.h"
#include "stinger_net/send_rcv.h"

using namespace gt::stinger;

void
RMAT (int64_t scale, double a, double b, double c, double d, int64_t *start, int64_t *end);


#endif /* _RMAT_EDGE_GENERATOR_H */

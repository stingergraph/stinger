#ifndef _RMAT_EDGE_GENERATOR_H
#define _RMAT_EDGE_GENERATOR_H

#include "stinger_net/proto/stinger-batch.pb.h"
#include "stinger_net/send_rcv.h"

using namespace gt::stinger;


void
rmat_edge (int64_t * iout, int64_t * jout,
           int SCALE, double A, double B, double C, double D, dxor128_env_t * env);

#endif /* _RMAT_EDGE_GENERATOR_H */

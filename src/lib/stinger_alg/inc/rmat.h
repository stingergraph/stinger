#ifndef STINGER_RMAT_H_
#define STINGER_RMAT_H_

#include "random.h"

void
rmat_edge (int64_t * iout, int64_t * jout,
           int SCALE, double A, double B, double C, double D, dxor128_env_t * env);

#endif
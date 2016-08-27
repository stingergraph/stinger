#include "stinger_la.h"

eweight_t
stinger_add(eweight_t a, eweight_t b)
{
  return a + b;
}

eweight_t
stinger_mult(eweight_t a, eweight_t b)
{
  return a * b;
}

eweight_t
stinger_max(eweight_t a, eweight_t b)
{
  return a > b ? a : b;
}

eweight_t
stinger_min(eweight_t a, eweight_t b)
{
  return a < b ? a : b;
}

eweight_t
stinger_and(eweight_t a, eweight_t b)
{
  return a && b ? 1 : 0;
}

eweight_t
stinger_or(eweight_t a, eweight_t b)
{
  return a || b ? 1 : 0;
}

int
stinger_matvec(stinger_t * S, eweight_t * vec_in, int64_t nv, eweight_t * vec_out, 
  eweight_t (*multiply)(eweight_t, eweight_t), eweight_t (*reduce)(eweight_t, eweight_t), 
  eweight_t empty)
{
  OMP("omp parallel for")
  for(int64_t v = 0; v < nv; v++) {
    eweight_t out = empty;
    eweight_t cur = empty;
    STINGER_FORALL_OUT_EDGES_OF_VTX_BEGIN(S, v) {
      if(STINGER_EDGE_DEST < nv) {
        out = reduce(multiply(STINGER_EDGE_WEIGHT, vec_in[STINGER_EDGE_DEST]), out);
      }
    } STINGER_FORALL_OUT_EDGES_OF_VTX_END();
    vec_out[v] = out;
  }
}


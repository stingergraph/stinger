#include    "random.h"

double dxor128(dxor128_env_t * e) {
  unsigned t=e->x^(e->x<<11);
  e->x=e->y; e->y=e->z; e->z=e->w; e->w=(e->w^(e->w>>19))^(t^(t>>8));
  return e->w*(1.0/4294967296.0);
}

void dxor128_init(dxor128_env_t * e) {
  e->x=123456789;
  e->y=362436069;
  e->z=521288629;
  e->w=88675123;
}

void dxor128_seed(dxor128_env_t * e, unsigned seed) {
  e->x=123456789;
  e->y=362436069;
  e->z=521288629;
  e->w=seed;
}

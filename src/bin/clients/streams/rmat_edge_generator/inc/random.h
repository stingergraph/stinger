typedef struct dxor128_env {
  unsigned x,y,z,w;
} dxor128_env_t;

double dxor128(dxor128_env_t * e);

void dxor128_init(dxor128_env_t * e);

void dxor128_seed(dxor128_env_t * e, unsigned seed);
